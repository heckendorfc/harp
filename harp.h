/*
 *  Copyright (C) 2009  Christian Heckendorf <vaseros@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_LIBASOUND
	#include <alsa/asoundlib.h>
#else
	#include <sys/fcntl.h>
	#include <sys/ioctl.h>
	#include <sys/soundcard.h>
#endif

#include <ctype.h>
#include <dlfcn.h>
#include <getopt.h>
#include <glob.h>
//math.h only used for ceil. use macro instead?
//#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <sqlite3.h>

#include "sndutil.h"

#define DB_PATH "~/.harp/"
#define DB "harp.db"

sqlite3 *conn;
struct dbitem{
	char **result;	  // All data
	char **row;		  // A row of data (pointers to result strings)
	int current_row;  // Row to read on the next fetch
	int row_count;    // Total data rows (header not included)
	int column_count; // Total columns
	char *err;
};

struct musicInfo{
	char *title;
	char *track;
	char *album;
	char *year;
	char *artist;
	char *length;
};

struct argument{
	char arg;
	char *subarg;
	unsigned int active;
};

char subarglist[]={
	'p',	/*Playlist*/
	's',	/*Song*/
	'a',	/*Album*/
	'r',	/*Artist*/
	0		/*Placeholder*/
};

struct argument arglist[]={
	{'l',NULL,0},	/*List*/
	{'p',NULL,0},	/*Play*/
	{'s',NULL,0},	/*Shuffle*/
	{'i',NULL,0},	/*Insert*/
	{'e',NULL,0},	/*Merge*/
	{'v',NULL,0},	/*Debug (verbose)*/
	{'t',NULL,0},	/*Type*/
	{'z',NULL,0},	/*Shuffle*/
	{'a',NULL,0},	/*Admin*/
	{0,  NULL,0}	/*Placeholder*/
};

struct option longopts[]={
	{"list",2,NULL,'l'},
	{"play",1,NULL,'p'},
	{"shuffle",0,NULL,'s'},
	{"insert",2,NULL,'i'},
	{"edit",1,NULL,'e'},
	{"verbose",0,NULL,'v'},
	{"type",1,NULL,'t'},
	{"zshuffle",0,NULL,'z'},
	{"admin",0,NULL,'a'},
	{0,0,0,0}
};

enum cmdarg{
	ALIST,
	APLAY,
	ASHUFFLE,
	AINSERT,
	AEDIT,
	AVERBOSE,
	ATYPE,
	AZSHUFFLE,
	AADMIN
};


enum filetype{
	FMP3=1,
	FASF,
	FM4A,
	FVOR,
	FFLAC
};

struct lconf{
	int maxwidth;
	int exception;
}listconf;

struct playercontrolarg{
	char *key;
	struct termios orig;
	struct playerHandles *ph;
	struct dbitem *listdbi;
	void *decoder;
};

//harp.c
void segv_leave(int sig);
void int_leave(int sig);
void cleanExit();

//argparse.c
unsigned int doArgs(int argc, char *argv[]);
unsigned int argSearch(int argc, char *argv[]);

//dbact.c
unsigned int dbInit();
void dbiInit(struct dbitem *dbi);
void dbiClean(struct dbitem *dbi);
int fetch_row(struct dbitem *dbi);
int fetch_row_at(struct dbitem *dbi, int index);
char ** fetch_column_at(struct dbitem *dbi, int index);
int doQuery(const char *querystr,struct dbitem *dbi);
int doTitleQuery(const char *querystr,struct dbitem *dbi,int *exception, int maxwidth);
void createTempPlaylistSong();

//insert.c
void db_safe(char *str, char *data, size_t size);
int getArtist(char *arg);
int getAlbum(char *arg, int id);
int getSong(char *arg, char *loc, int id);
int getPlaylist(char *arg);
int verifySong(int sid);
int getPlaylistSong(int sid,int pid);
int batchInsert(char *arg);
unsigned int insertSong(char *arg);

//edit.c
int batchEdit(int *ids,int len);

//util.c
void setDefaultConfig();
char *expand(char *in);
int fileFormat(char *argv);
int isNumeric(char *argv);
int getPlugin(struct dbitem *dbi, int index, void **module);
int getID(char *arg);
int strToID(char *argv);
char *getFilename(char *path);
int *getMulti(char *arg, int *length);
void miClean(struct musicInfo *mi);

//message.c
void debug(char *argv);
void debug3(char *argv);
void debugf(char *msg, int cnt, ...);
void printSongPubInfo(char **row);

//player.c
int player(int list);
void playerControl(void *arg);
int getSystemKey(char key,struct playercontrolarg *pca);
void shuffle(int list);
void zshuffle(int list);
void shuffleCleanup(int list);

//list.c
int list(int id);
int listall();

//admin.c
void adminPortal();

#include "dbact.c"
#include "argparse.c"
#include "insert.c"
#include "edit.c"
#include "util.c"
#include "message.c"
#include "player.c"
#include "list.c"
#include "admin.c"
#include "config.c"

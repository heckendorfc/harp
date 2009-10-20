/*
 *  Copyright (C) 2009  Christian Heckendorf <heckendorfc@gmail.com>
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

#if WITH_ALSA==1
	#include <alsa/asoundlib.h>
#elif WITH_JACK==1
	#include <jack/jack.h>
#else
	#include <sys/fcntl.h>
	#include <sys/ioctl.h>
	#include <sys/soundcard.h>
#endif

#include <ctype.h>
#include <dlfcn.h>
#include <getopt.h>
#include <glob.h>
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
#define DB_BATCH_SIZE 100

#define MI_TITLE_SIZE 200
#define MI_TRACK_SIZE 9
#define MI_ARTIST_SIZE 100
#define MI_ALBUM_SIZE 100
#define MI_YEAR_SIZE 4
#define MI_LENGTH_SIZE 8

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
	'g',	/*Genre*/
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

struct insert_data{
	const int fmt;
	char *query;
	struct musicInfo *mi;
	struct dbitem *dbi;
	const char *path;
};

struct lconf{
	int maxwidth;
	int exception;
}listconf;

struct iconf{
	char **f_root;
	char **format;
	int format_length;
	int (*first_cb)(struct insert_data *data);
	int (*second_cb)(struct insert_data *data);
}insertconf;

struct playercontrolarg{
	char *key;
	unsigned int cur_order;
	unsigned int next_order;
	struct termios orig;
	struct playerHandles *ph;
	void *decoder;
};

struct commandOption{
	char opt;
	int (*function)(char *args, void *data);
	const char *help;
	void *data;
};

struct dbnode{
	struct dbitem dbi;
	struct dbnode *prev;
	struct dbnode *next;
	int depth;
};

struct titlequery_data{
	int count;
	int maxwidth;
	int *exception;
	int *exlen;
};

//harp.c
void int_leave(int sig);
void cleanExit();

//argparse.c
unsigned int doArgs(int argc, char *argv[]);

//dbact.c
unsigned int dbInit();
void dbiInit(struct dbitem *dbi);
void dbiClean(struct dbitem *dbi);
int fetch_row(struct dbitem *dbi);
int fetch_row_at(struct dbitem *dbi, int index);
char ** fetch_column_at(struct dbitem *dbi, int index);
int doQuery(const char *querystr,struct dbitem *dbi);
int uint_return_cb(void *arg, int col_count, char **row, char **titles);
int str_return_cb(void *arg, int col_count, char **row, char **titles);
int doTitleQuery(const char *querystr,int *exception, int maxwidth);
void createTempPlaylistSong();

//insert.c
int getArtist(const char *arg);
int getAlbum(const char *arg, int id);
int getSong(const char *arg, const char *loc, const int id);
int getPlaylist(const char *arg);
int getCategory(const char *arg);
int getPlaylistSong(const int sid, const int pid);
int getSongCategory(const int sid, const int cid);
int batchInsert(char *arg);
int filepathInsert(struct insert_data *data);
int metadataInsert(struct insert_data *data);

//edit.c
void editPortal();

//util.c
void setDefaultConfig();
char *expand(char *in);
int fileFormat(const char *argv);
int getPlugin(struct dbitem *dbi, const int index, void **module);
int getID(const char *arg);
int strToID(const char *argv);
char *getFilename(const char *path);
int *getMulti(char *arg, int *length);
void cleanTempSelect(const int tempid);
int insertTempSelect(const int *ids, const int idlen);
void miFree(struct musicInfo *mi);
void miClean(struct musicInfo *mi);
void db_clean(char *str, const char *data, const size_t size);
void db_safe(char *str, const char *data, const size_t size);

//message.c
void debug(const int level, const char *msg);
void printSongPubInfo(char **row);

//player.c
int player(int list);
void playerControl(void *arg);
int getSystemKey(char key,struct playercontrolarg *pca);

//shuffle.c
void shuffle(int list);
void zshuffle(unsigned int items);
void shuffleCleanup(int list);

//list.c
int list(int *ids, int length);
int listall();

//admin.c
void adminPortal();
int write_stats_cb(void *data, int col_count, char **row, char **titles);

//portal.c
void cleanString(char **ostr);
int editWarn(char *warn);
int getStdArgs(char *args,char *prompt);
int portal(struct commandOption *portalOptions, const char *prefix);

//tree.c
struct dbnode *dbnodeAdd(struct dbnode *node);
struct dbnode *dbnodeClean(struct dbnode *node);
int *getGenreHeadPath(int head);
void printGenreHeadPath(int *path);
void printGenreChildren(struct dbnode *cur, int curid, void *action(struct dbnode*));
void tierChildPrint(struct dbnode *cur);
void tierCatPrint(struct dbnode *cur);
void printGenreTree(int head, void *action(struct dbnode *));

#include "dbact.c"
#include "argparse.c"
#include "insert.c"
#include "edit.c"
#include "util.c"
#include "message.c"
#include "shuffle.c"
#include "player.c"
#include "list.c"
#include "admin.c"
#include "config.c"
#include "portal.c"
#include "tree.c"

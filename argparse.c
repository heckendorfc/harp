/*
 *  Copyright (C) 2009-2016  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "argparse.h"
#include "defs.h"
#include "dbact.h"
#include "harpconfig.h"
#include "list.h"
#include "shuffle.h"
#include "player.h"
#include "insert.h"
#include "admin.h"
#include "util.h"
#include "harp.h"
#include "tree.h"
#include "edit_shell.h"
#include "edit.h"

#if 0
static char subarglist[]={
	'p',	/*Playlist*/
	's',	/*Song*/
	'a',	/*Album*/
	'r',	/*Artist*/
	'g',	/*Genre*/
	0		/*Placeholder*/
};
#endif

struct argument arglist[]={
	{'l',NULL,0},	/*List*/
	{'p',NULL,0},	/*Play*/
	{'s',NULL,0},	/*Shuffle*/
	{'i',NULL,0},	/*Insert*/
	{'e',NULL,0},	/*Merge*/
	{'E',NULL,0},	/*Merge*/
	{123,NULL,0},	/*Merge*/
	{'v',NULL,0},	/*Debug (verbose)*/
	{'t',NULL,0},	/*Type*/
	{'z',NULL,0},	/*Shuffle*/
	{'a',NULL,0},	/*Admin*/
	{'D',NULL,0},	/*Device*/
	{'r',NULL,0},	/*Repeat*/
	{0,  NULL,0}	/*Placeholder*/
};

struct option longopts[]={
	{"list",2,NULL,'l'},
	{"play",1,NULL,'p'},
	{"shuffle",2,NULL,'s'},
	{"insert",2,NULL,'i'},
	{"edit-stdin",0,NULL,'e'},
	{"edit",1,NULL,'E'},
	{"old-edit",0,NULL,123},
	{"verbose",0,NULL,'v'},
	{"type",1,NULL,'t'},
	{"zshuffle",0,NULL,'z'},
	{"admin",0,NULL,'a'},
	{"device",2,NULL,'D'},
	{"repeat",0,NULL,'r'},
	{"version",0,NULL,200},
	{0,0,0,0}
};

static unsigned int argSearch(int argc, char *argv[]);

static void printVersion(){
	printf("HARP %s  Copyright (C) 2009-2015 Christian Heckendorf\n",PACKAGE_VERSION);
	cleanExit();
	exit(1);
}

static void printHelp(){
	printf("Valid options are:\n\
	-t [s,p,r,a,g,t]\tType (song, playlist, artist, album, genre, tag)\n\
	-l [name, ID]\tList (requires -t)\n\
	-p [name, ID]\tPlay (requires -t)\n\
		-s [s,a,r]\tShuffle (requires -p)\n\
		-z\tSmart shuffle (requires -p)\n\
		-r [number of repeats]\tRepeat playlist (requires -p)\n\
		-D {device name}\tOutput to specified device (requires -p)\n\
	-i [file path, directory]\tInsert songs\n\
	-e\tEdit via script using stdin\n\
	-E {script}\tEdit via script using command line\n\
	--old-edit\tEdit via deprecated portal interface\n\
	-a\tAdmin\n\
	-v\tVerbose\n\
	--version\t Print version information\n");
	cleanExit();
	exit(1);
}

static void genreToPlaylistSong(struct dbnode *cur){
	char query[150],cb_query[150];
	static struct insert_tps_arg data={1,0,NULL,0};

	if(!cur->dbi.row_count)
		return;

	data.query=cb_query;
	data.querysize=150;

	snprintf(query,150,"SELECT Song.SongID FROM SongCategory NATURAL JOIN Song WHERE Active=1 AND CategoryID=%s",cur->dbi.row[0]);
	harp_sqlite3_exec(conn,query,batch_tempplaylistsong_insert_cb,&data,NULL);
}

static void makeTempPlaylist(int *multilist, int multi){
	int mx;
	char query[250],cb_query[150];
	struct insert_tps_arg data={1,0,cb_query,150};

	createTempPlaylistSong();

	switch(*arglist[ATYPE].subarg){
		case 'p':
			for(mx=0;mx<multi;mx++){
				snprintf(query,250,"SELECT Song.SongID FROM PlaylistSong NATURAL JOIN Song WHERE Active=1 AND PlaylistID=%d ORDER BY \"Order\"",multilist[mx]);
				harp_sqlite3_exec(conn,query,batch_tempplaylistsong_insert_cb,&data,NULL);
			}
			break;
		case 's':
			for(mx=0;mx<multi;mx++){
				snprintf(query,250,"SELECT SongID FROM Song WHERE SongID=%d",multilist[mx]);
				harp_sqlite3_exec(conn,query,batch_tempplaylistsong_insert_cb,&data,NULL);
			}
			break;
		case 'a':
			for(mx=0;mx<multi;mx++){
				//sprintf(query,"SELECT Song.SongID FROM Album INNER JOIN Song USING(AlbumID) WHERE Active=1 AND Album.AlbumID=%d ORDER BY Track",multilist[mx]);
				snprintf(query,250,"SELECT SongID FROM Song WHERE Active=1 AND AlbumID=%d ORDER BY Track",multilist[mx]);
				harp_sqlite3_exec(conn,query,batch_tempplaylistsong_insert_cb,&data,NULL);
			}
			break;
		case 'r':
			for(mx=0;mx<multi;mx++){
				snprintf(query,250,"SELECT Song.SongID FROM AlbumArtist NATURAL JOIN Song WHERE Active=1 AND AlbumArtist.ArtistID=%d ORDER BY AlbumArtist.AlbumID,Track",multilist[mx]);
				harp_sqlite3_exec(conn,query,batch_tempplaylistsong_insert_cb,&data,NULL);
			}
			break;
		case 't':
			for(mx=0;mx<multi;mx++){
				snprintf(query,250,"SELECT Song.SongID FROM SongTag NATURAL JOIN Song WHERE Active=1 AND TagID=%d ORDER BY TagID,Track",multilist[mx]);
				harp_sqlite3_exec(conn,query,batch_tempplaylistsong_insert_cb,&data,NULL);
			}
			break;
		case 'g':
			for(mx=0;mx<multi;mx++){
				printGenreTree(multilist[mx],(void *)genreToPlaylistSong);
			}
			break;
	}
}

unsigned int doArgs(int argc,char *argv[]){
	int *multilist,id=0,multi=0;
	setDefaultConfig();

	if(argSearch(argc,argv)==0){
		return 0;
	}

	//list
	if(arglist[ALIST].active){
		if(!arglist[ALIST].subarg){
			listall();
			return 0;
		}

		if(!(multilist=getMulti(arglist[ALIST].subarg,&multi)))return 1;
		if(multi<1 || *multilist<1)return 1;
		list(multilist,multi);
		free(multilist);
		return 0;
	}
	//play
	if(arglist[APLAY].active){
		if(!(multilist=getMulti(arglist[APLAY].subarg,&multi)))return 1;
		if(multi<1 || *multilist<1)return 1;

		makeTempPlaylist(multilist,multi);
		id=0;

		free(multilist);

		if(arglist[AREPEAT].active && arglist[AREPEAT].subarg!=NULL)
			arglist[AREPEAT].active=strtol(arglist[AREPEAT].subarg,NULL,10);

		if(arglist[ASHUFFLE].active || arglist[AZSHUFFLE].active){
			shuffle(id);
			id=0;
		}
		player(id);
		return 0;
	}
	//insert
	if(arglist[AINSERT].active){
		batchInsert(arglist[AINSERT].subarg);
		return 0;
	}
	//edit
	if(arglist[AEDIT].active){
		switch(arglist[AEDIT].active){
			case 1:
				editShell();
				break;
			case 2:
				editPortal();
				break;
		}
		return 0;
	}
	//admin
	if(arglist[AADMIN].active){
		adminPortal();
		return 0;
	}
	printHelp();
	return 0;
}

static unsigned int argSearch(int argc,char *argv[]){
	int opt,optindex;
	while((opt=getopt_long(argc,argv,"i::l::E:D::p:s::t:evzar::",longopts,&optindex))!=-1){
		switch(opt){
			//case 'e':arglist[AEDIT].active=1;arglist[AEDIT].subarg=optarg;break;
			case 123:arglist[AEDIT].active=2;break;
			case 'e':arglist[AEDIT].active=1;arglist[AEDIT].subarg=NULL;break;
			case 'E':arglist[AEDIT].active=1;arglist[AEDIT].subarg=optarg;break;
			case 'i':arglist[AINSERT].active=1;arglist[AINSERT].subarg=(argv[optind]&&argv[optind][0]!='-')?argv[optind]:optarg;break;
			case 'l':arglist[ALIST].active=1;arglist[ALIST].subarg=(argv[optind]&&argv[optind][0]!='-')?argv[optind]:optarg;break;
			case 'D':arglist[ADEVICE].active=1;arglist[ADEVICE].subarg=(argv[optind]&&argv[optind][0]!='-')?argv[optind]:optarg;break;
			case 'p':arglist[APLAY].active=1;arglist[APLAY].subarg=optarg;break;
			case 's':arglist[ASHUFFLE].active=1;arglist[AZSHUFFLE].active=0;arglist[ASHUFFLE].subarg=(argv[optind]&&argv[optind][0]!='-')?argv[optind]:optarg;break;
			case 't':arglist[ATYPE].active=1;arglist[ATYPE].subarg=optarg;break;
			case 'v':arglist[AVERBOSE].active++;break;
			case 'z':arglist[AZSHUFFLE].active=1;arglist[ASHUFFLE].active=0;break;
			case 'a':arglist[AADMIN].active=1;break;
			case 'r':arglist[AREPEAT].active=1;arglist[AREPEAT].subarg=(argv[optind]&&argv[optind][0]!='-')?argv[optind]:optarg;break;
			case 200:printVersion();break;
			case '?':
			default: printHelp();break;
		}
	}
	return 1;
}

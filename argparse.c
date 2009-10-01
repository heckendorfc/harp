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

static unsigned int argSearch(int argc, char *argv[]);

static void printHelp(){
	printf("Valid options are:\n\
	t [s,p,r,a]\tType (song, playlist, artist, album)\n\
	l [name, ID]\tList (requires -t)\n\
	p [name, ID]\tPlay (requires -t)\n\
		s\tShuffle (requires -p)\n\
		z\tSmart shuffle (requires -p)\n\
	i [name]\tInsert (requires -t)\n\
	e [name, ID]\tEdit (requires -t)\n\
	a\tAdmin\n\
	v\tVerbose\n");
	cleanExit();
	exit(1);
}

static void insertPS(char *query,struct dbitem *dbi){
	static int order=1;
	int currentlimit=0;
	doQuery(query,dbi);

	dbi->row_count++;
	int x=1;
	while(x<dbi->row_count){
		if((currentlimit+=100)>dbi->row_count)currentlimit=dbi->row_count;
		sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
		for(;x<currentlimit;x++){
			sprintf(query,"INSERT INTO TempPlaylistSong(SongID,`Order`) VALUES(%s,%d)",dbi->result[x],order+x);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
	}
	order+=x-1;
}

static void genreToPlaylistSong(struct dbnode *cur){
	int x;
	if(!cur->dbi.row_count)return;
	char query[100];
	sprintf(query,"SELECT Song.SongID FROM SongCategory NATURAL JOIN Song WHERE Active=1 AND CategoryID=%d",(int)strtol(cur->dbi.row[0],NULL,10));
	debug(3,query);
	// TODO: See if order will auto increment. It would be nice to skip this function call.
	insertPS(query,&cur->dbi);
}

static void makeTempPlaylist(int *multilist, int multi){
	int mx,x,order=0,currentlimit=0;
	char query[250];
	struct dbitem dbi;
	dbiInit(&dbi);

	createTempPlaylistSong();

	switch(*arglist[ATYPE].subarg){
		case 'p':
			for(mx=0;mx<multi;mx++){
				sprintf(query,"SELECT Song.SongID FROM PlaylistSong NATURAL JOIN Song WHERE Active=1 AND PlaylistID=%d",multilist[mx]);
				insertPS(query,&dbi);
			}
			break;
		case 's':
			for(mx=0;mx<multi;mx++){
				sprintf(query,"SELECT SongID FROM Song WHERE SongID=%d",multilist[mx]);
				insertPS(query,&dbi);
			}
			break;
		case 'a':
			for(mx=0;mx<multi;mx++){
				sprintf(query,"SELECT Song.SongID FROM Song,Album WHERE Song.AlbumID=Album.AlbumID AND Active=1 AND Album.AlbumID=%d",multilist[mx]);
				insertPS(query,&dbi);
			}
			break;
		case 'r':
			for(mx=0;mx<multi;mx++){
				sprintf(query,"SELECT Song.SongID FROM Song,AlbumArtist WHERE Song.AlbumID=AlbumArtist.AlbumID AND Active=1 AND AlbumArtist.ArtistID=%d",multilist[mx]);
				insertPS(query,&dbi);
			}
			break;
		case 'g':
			for(mx=0;mx<multi;mx++){
				printGenreTree(multilist[mx],(void *)genreToPlaylistSong);
			}
			break;
	}
	dbiClean(&dbi);
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

		multilist=getMulti(arglist[ALIST].subarg,&multi);
		if(multi<1 || *multilist<1)return 1;
		list(multilist,multi);
		free(multilist);
		return 0;
	}
	//play
	if(arglist[APLAY].active){
		multilist=getMulti(arglist[APLAY].subarg,&multi);
		if(multi<1 || *multilist<1)return 1;
		if(multi>0 || *arglist[ATYPE].subarg!='p'){ // Skip for single playlists
			makeTempPlaylist(multilist,multi);
			id=0;
		}
		else{
			id=multilist[0];
		}
		free(multilist);

		if(arglist[AZSHUFFLE].active){
			zshuffle(id);
			id=0;
		}
		else if(arglist[ASHUFFLE].active){
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
		editPortal();
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
	while((opt=getopt_long(argc,argv,"i::l::e::p:st:vza",longopts,&optindex))!=-1){
		switch(opt){
			//case 'e':arglist[AEDIT].active=1;arglist[AEDIT].subarg=optarg;break;
			case 'e':arglist[AEDIT].active=1;arglist[AEDIT].subarg=(argv[optind]&&argv[optind][0]!='-')?argv[optind]:optarg;break;
			case 'i':arglist[AINSERT].active=1;arglist[AINSERT].subarg=(argv[optind]&&argv[optind][0]!='-')?argv[optind]:optarg;break;
			case 'l':arglist[ALIST].active=1;arglist[ALIST].subarg=(argv[optind]&&argv[optind][0]!='-')?argv[optind]:optarg;break;
			case 'p':arglist[APLAY].active=1;arglist[APLAY].subarg=optarg;break;
			case 's':arglist[ASHUFFLE].active=1;arglist[AZSHUFFLE].active=0;break;
			case 't':arglist[ATYPE].active=1;arglist[ATYPE].subarg=optarg;break;
			case 'v':arglist[AVERBOSE].active++;break;
			case 'z':arglist[AZSHUFFLE].active=1;arglist[ASHUFFLE].active=0;break;
			case 'a':arglist[AADMIN].active=1;break;
			case '?':
			default: printHelp();break;
		}
	}
	return 1;
}

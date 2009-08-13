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


void printHelp(){
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

void makeTempPlaylist(int *multilist, int multi){
	int mx,x,order=0,currentlimit=0;
	char query[250];
	struct dbitem dbi;
	dbiInit(&dbi);

	createTempPlaylistSong();

	for(mx=0;mx<multi;mx++){
		switch(*arglist[ATYPE].subarg){
			case 'p':
				sprintf(query,"SELECT Song.SongID FROM PlaylistSong NATURAL JOIN Song WHERE Active=1 AND PlaylistID=%d",multilist[mx]);break;
			case 's':
				sprintf(query,"SELECT SongID FROM Song WHERE SongID=%d",multilist[mx]);break;
			case 'a':
				sprintf(query,"SELECT Song.SongID FROM Song,Album WHERE Song.AlbumID=Album.AlbumID AND Active=1 AND Album.AlbumID=%d",multilist[mx]);break;
			case 'r':
				sprintf(query,"SELECT Song.SongID FROM Song,AlbumArtist WHERE Song.AlbumID=AlbumArtist.AlbumID AND Active=1 AND AlbumArtist.ArtistID=%d",multilist[mx]);break;
		}
		doQuery(query,&dbi);

		dbi.row_count++;
		x=1;
		while(x<dbi.row_count){
			if((currentlimit+=100)>dbi.row_count)currentlimit=dbi.row_count;
			sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
			for(;x<currentlimit;x++){
				sprintf(query,"INSERT INTO TempPlaylistSong(SongID,`Order`) VALUES(%s,%d)",dbi.result[x],order+x);
				sqlite3_exec(conn,query,NULL,NULL,NULL);
			}
			sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
		}
		order+=x-1;
		currentlimit=0;
	}
	dbiClean(&dbi);
}

unsigned int doArgs(int argc,char *argv[]){
	setDefaultConfig();

	if(argSearch(argc,argv)==0){
		return 0;
	}
	debugf("Verbose level:",1,arglist[AVERBOSE].active);
	int *multilist,id=0,multi=0;
	
	//list
	if(arglist[ALIST].active){
		if(!arglist[ALIST].subarg){
			listall();
			return 0;
		}

		if((id=getID(arglist[ALIST].subarg))<0)return 1;
		list(id);
		return 0;
	}
	//play
	if(arglist[APLAY].active){
		multilist=getMulti(arglist[APLAY].subarg,&multi);
		if(multi<1 || *multilist<0)return 1;
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
		multilist=getMulti(arglist[AEDIT].subarg,&multi);
		if(*multilist>-1)
			batchEdit(multilist,multi);
		free(multilist);
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

unsigned int argSearch(int argc,char *argv[]){
	int opt,optindex;
	while((opt=getopt_long(argc,argv,"i::l::e:p:st:vza",longopts,&optindex))!=-1){
		switch(opt){
			case 'e':arglist[AEDIT].active=1;arglist[AEDIT].subarg=optarg;break;
	//		case 'i':arglist[AINSERT].active=1;arglist[AINSERT].subarg=optarg;break;
			case 'i':arglist[AINSERT].active=1;arglist[AINSERT].subarg=(argv[optind]&&argv[optind][0]!='-')?argv[optind]:optarg;break;
	//		case 'l':arglist[ALIST].active=1;arglist[ALIST].subarg=optarg;break;
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

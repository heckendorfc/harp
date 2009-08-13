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


void db_insert_safe(char *str, char *data, size_t size){
	if(strcmp(data,"")==0){strcpy(str,"Unknown");return;}
	int x,z=0;
	for(x=0;data[x]>31 && data[x]<127 && x<size;x++){//strip multi space
		if(data[x]==' ' && data[x+1]==' ')continue;
		str[z]=data[x];
		z++;
	}
	str[z]=0;
	for(x=0;str[x]!='\0';x++){//addslashes
		//if(str[x]=='\'' || str[x]=='\"'){
		if(str[x]=='\''){
			memmove(&str[x+1],&str[x],(z+1)-x);
			str[x]='\'';
			x++;
			z++;
		}
	}
	//fprintf(stderr,"%s\n",str);
	//if(str[x-1]==' ')//strip trailing space
	//	str[x-1]=0;
	if(strcmp(str,"")==0)strcpy(str,"Unknown");
}

void db_safe(char *str, char *data, size_t size){
	int x,z=0;
	for(x=0;data[x]>31 && data[x]<127 && x<size;x++){//strip multi space
		if(data[x]==' ' && data[x+1]==' ')continue;
		str[z]=data[x];
		z++;
	}
	str[z]=0;
	for(x=0;str[x]!='\0';x++){//addslashes
		//if(str[x]=='\'' || str[x]=='\"'){
		if(str[x]=='\''){
			memmove(&str[x+1],&str[x],(z+1)-x);
			str[x]='\'';
			x++;
			z++;
		}
	}
	//fprintf(stderr,"%s\n",str);
	//if(str[x-1]==' ')//strip trailing space
	//	str[x-1]=0;
}
int getArtist(char *arg){//doesn't handle duplicate name entries
	char query[301];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(ArtistID), ArtistID FROM Artist WHERE Name=\'%s\'",arg);
	debug3(query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create artist
		sprintf(query,"INSERT INTO Artist (Name) VALUES (\'%s\')",arg);
		debug3(query);
		if(doQuery(query,&dbi)<1){
			dbiClean(&dbi);
			return -1;
		}
		dbiClean(&dbi);
		return sqlite3_last_insert_rowid(conn);
	}
	int id=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return id;
}

int getAlbum(char *arg,int id){//doesn't handle duplicate name entries
	char query[401];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(Album.AlbumID), Album.AlbumID FROM Album,AlbumArtist WHERE Album.Title=\'%s\' AND Album.AlbumID = AlbumArtist.AlbumID AND AlbumArtist.ArtistID=%d",arg,id);
	debug3(query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create album
		sprintf(query,"INSERT INTO Album (Title) VALUES (\'%s\')",arg);
		debug3(query);
		if(doQuery(query,&dbi)<1){
			dbiClean(&dbi);
			return -1;
		}
		sprintf(query,"INSERT INTO AlbumArtist (ArtistID,AlbumID) VALUES (%d,%d)",id,(int)sqlite3_last_insert_rowid(conn));
		debug3(query);
		if(doQuery(query,&dbi)<1){
			dbiClean(&dbi);
			return -1;
		}
		dbiClean(&dbi);
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

int getSong(char *arg,char *loc,int id){//doesn't handle duplicate name entries
	char query[401];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(Song.SongID), Song.SongID FROM Song, Album WHERE Song.Location=\'%s\' AND Song.AlbumID = Album.AlbumID AND Album.AlbumID=%d",loc,id);
	debug3(query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create song
		sprintf(query,"INSERT INTO Song (Title,Location,AlbumID,TypeID) VALUES (\'%s\',\'%s\',%d,%d)",arg,loc,id,-1);
		debug3(query);
		if(doQuery(query,&dbi)<1){
			dbiClean(&dbi);
			return -1;
		}
		dbiClean(&dbi);
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

int getPlaylist(char *arg){
	char query[401];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(Playlist.PlaylistID), Playlist.PlaylistID FROM Playlist WHERE Playlist.Title=\'%s\'",arg);
	debug3(query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create playlist
		dbiClean(&dbi);
		sprintf(query,"INSERT INTO Playlist (Title) VALUES ('%s')",arg);
		debug3(query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK){
			return -1;
		}
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

int getPlaylistSong(int sid, int pid){
	char query[201];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(PlaylistSong.PlaylistSongID), PlaylistSong.PlaylistSongID FROM PlaylistSong WHERE PlaylistSong.PlaylistID=%d AND PlaylistSong.SongID=%d",pid,sid);
	debug3(query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create playlistsong
		//sprintf(query,"SELECT COUNT(PlaylistSong.PlaylistSongID) FROM PlaylistSong WHERE PlaylistSong.PlaylistID=%d",pid);
		sprintf(query,"SELECT \"Order\" FROM PlaylistSong WHERE PlaylistID=%d ORDER BY \"Order\" DESC LIMIT 1",pid);
		debug3(query);
		doQuery(query,&dbi);
		int order;
		if(fetch_row(&dbi))
			order=(int)strtol(dbi.row[0],NULL,10)+1;
		else
			order=1;

		sprintf(query,"INSERT INTO PlaylistSong(SongID,PlaylistID,\"Order\") VALUES(%d,%d,%d)",sid,pid,order);
		debug3(query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK){
			dbiClean(&dbi);
			return -1;
		}
		dbiClean(&dbi);
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

int verifySong(int sid){
	char query[201];
	char ans[4];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT Song.Title,Song.Location FROM Song WHERE Song.SongID=%d",sid);
	printf("%s\n",query);
	doQuery(query,&dbi);
	if(fetch_row(&dbi)){
		printf("%s located at: %s.\nIs this correct?[y] ",dbi.row[0],dbi.row[1]);
		fgets(ans,sizeof(ans),stdin);
		dbiClean(&dbi);
		if(*ans=='y' || *ans=='\n')
			return sid;
		else
			return -1;
	}
	else
		fprintf(stderr,"That song doesn't exist\n");
	dbiClean(&dbi);
	return -1;
}

	

int batchInsert(char *arg){
	if(*arglist[ATYPE].subarg=='s'){//song
		if(arg){//single argv insert
			debug("Inserting song at: ");
			debug(expand(arg));
			return insertSong(arg);
		}
		else{//batch insert
			char temp[250];
			while(printf("File Location: ") && fgets(temp,sizeof(temp),stdin) && *temp!='\n'){
				expand(temp);
				insertSong(temp);
			}
			debug("Done.");
			return 1;
		}
	}
	if(*arglist[ATYPE].subarg=='p'){//playlist
		char temp[100],tempc[11],*nl;
		int pid,sid;
		if(arg){ // Get name from insert subarg
			strncpy(temp,arg,sizeof(temp));
		}
		else{ // Get name from prompt
			printf("Name: ");
			fgets(temp,sizeof(temp),stdin);
			if((nl=strchr(temp,'\n')))*nl=0;
		}
		printf("Inserting playlist: %s\n",temp);
		pid=getPlaylist(temp);
		if(pid<1){
			fprintf(stderr,"Error inserting playlist\n");
			return 0;
		}
		while(printf("SongID: ") && fgets(temp,sizeof(temp),stdin) && *temp!='\n'){
			if((sid=getID(temp))==-1)continue;
			sid=verifySong(sid);
			if(sid>0)
				getPlaylistSong(sid,pid);
			printf("Success.\n");
			sid=0;
		}
	}
	return 0;

}

typedef struct musicInfo * (*function_meta)(FILE *ffd);

unsigned int insertSong(char *arg){
	//chack for dupicate
	struct dbitem dbi;
	dbiInit(&dbi);
	char dbq[350];

	char dbfilename[250];
	int x;for(x=0;arg[x]!=0;x++);
	db_insert_safe(dbfilename,arg,x);
	//fprintf(stderr,"\n%d %s\n",x,argv);
	
	sprintf(dbq,"SELECT SongID FROM Song WHERE Location='%s'",dbfilename);
	doQuery(dbq,&dbi);
	if(dbi.row_count){
		debug("duplicate entry -- skipping");
		dbiClean(&dbi);
		return 0;
	}
	debug("Finding file type");
	int fmt;
	if((fmt=fileFormat(arg))<1){
		dbiClean(&dbi);
		if(fmt==0)fprintf(stderr,"Unknown file type\n");
		return 0;
	}
	int songid=0;

	FILE *ffd;
	int artistid,albumid;
	char library[200];
	void *module;
	if((ffd=fopen(arg,"rb"))==NULL){
		dbiClean(&dbi);
		fprintf(stderr,"Can't open file\n");
		return 0;
	}

	function_meta modmeta;
	struct musicInfo *mi;
	sprintf(dbq,"SELECT Library FROM FilePlugin WHERE TypeID=%d",fmt);
	doQuery(dbq,&dbi);
	debug3(dbq);
	while((x=getPlugin(&dbi,0,&module))){
		if(x>1)continue;
		modmeta=dlsym(module,"plugin_meta");
		if(!modmeta){
			debug("Plugin does not contain plugin_meta().\n");
//			debug(dlerror());
			continue;
		}
		else{
			mi=modmeta(ffd);
			break;
		}
		dlclose(module);
	}
	fclose(ffd);
	if(!x){
		dbiClean(&dbi);
		return 0;
	}

	char tempname[255];
	if(mi->artist==NULL){
		mi->artist=malloc(sizeof(char)*8);
		strcpy(mi->artist,"Unknown");
	}
	else{
		db_insert_safe(tempname,mi->artist,255);
		strcpy(mi->artist,tempname);
	}
	if((artistid=getArtist(mi->artist))==-1){
		fprintf(stderr,"Error inserting artist.");
		miClean(mi);
		dbiClean(&dbi);
		return 0;
	}
	if(mi->album==NULL){
		mi->album=malloc(sizeof(char)*8);
		strcpy(mi->album,"Unknown");
	}
	else{
		db_insert_safe(tempname,mi->album,255);
		strcpy(mi->album,tempname);
	}
	if((albumid=getAlbum(mi->album,artistid))==-1){
		fprintf(stderr,"Error inserting album.");
		miClean(mi);
		dbiClean(&dbi);
		return 0;
	}

	if(mi->title==NULL){
		mi->title=malloc(sizeof(char)*100);
		strcpy(mi->title,getFilename(arg));
	}
	else{
		db_insert_safe(tempname,mi->title,255);
		strcpy(mi->title,tempname);
	}
	if((songid=getSong(mi->title,dbfilename,albumid))==-1){
		fprintf(stderr,"Error inserting song.");
		miClean(mi);
		dbiClean(&dbi);
		return 0;
	}
	miClean(mi);

	sprintf(dbq,"UPDATE Song SET TypeID=%d WHERE SongID=%d",fmt,songid);
	debug3(dbq);
	doQuery(dbq,&dbi);

	getPlaylistSong(songid,getPlaylist("Library"));
	debug("Song inserted into Library");
	return 1;
}

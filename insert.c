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

typedef void (*function_meta)(FILE *ffd, struct musicInfo *mi);
static int verifySong(const int sid);
static unsigned int insertSong(const char *arg, struct musicInfo *mi);

static void db_insert_safe(char *str, char *data, const size_t size){
	if(!*data){
		strcpy(str,"Unknown");
		return;
	}
	db_clean(str,data,size);
	strcpy(data,str);
	db_safe(str,data,size);
	if(!*str)strcpy(str,"Unknown");
}

int getArtist(const char *arg){
	char query[301];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(ArtistID), ArtistID FROM Artist WHERE Name=\'%s\'",arg);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create artist
		dbiClean(&dbi);
		sprintf(query,"INSERT INTO Artist (Name) VALUES (\'%s\')",arg);
		debug(3,query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		return sqlite3_last_insert_rowid(conn);
	}
	int id=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return id;
}

int getAlbum(const char *arg, const int id){
	char query[401];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(Album.AlbumID), Album.AlbumID FROM Album,AlbumArtist WHERE Album.Title=\'%s\' AND Album.AlbumID = AlbumArtist.AlbumID AND AlbumArtist.ArtistID=%d",arg,id);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create album
		dbiClean(&dbi);
		sprintf(query,"INSERT INTO Album (Title) VALUES (\'%s\')",arg);
		debug(3,query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		sprintf(query,"INSERT INTO AlbumArtist (ArtistID,AlbumID) VALUES (%d,%d)",id,(int)sqlite3_last_insert_rowid(conn));
		debug(3,query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

int getSong(const char *arg, const char *loc, const int id){
	char query[401];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(Song.SongID), Song.SongID FROM Song, Album WHERE Song.Location=\'%s\' AND Song.AlbumID = Album.AlbumID AND Album.AlbumID=%d",loc,id);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create song
		dbiClean(&dbi);
		sprintf(query,"INSERT INTO Song (Title,Location,AlbumID,TypeID) VALUES (\'%s\',\'%s\',%d,%d)",arg,loc,id,-1);
		debug(3,query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

int getPlaylist(const char *arg){
	char query[401];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(Playlist.PlaylistID), Playlist.PlaylistID FROM Playlist WHERE Playlist.Title=\'%s\'",arg);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create playlist
		dbiClean(&dbi);
		sprintf(query,"INSERT INTO Playlist (Title) VALUES ('%s')",arg);
		debug(3,query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

int getPlaylistSong(const int sid, const int pid){
	char query[201];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(PlaylistSong.PlaylistSongID), PlaylistSong.PlaylistSongID FROM PlaylistSong WHERE PlaylistSong.PlaylistID=%d AND PlaylistSong.SongID=%d",pid,sid);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create playlistsong
		sprintf(query,"SELECT \"Order\" FROM PlaylistSong WHERE PlaylistID=%d ORDER BY \"Order\" DESC LIMIT 1",pid);
		debug(3,query);
		doQuery(query,&dbi);
		int order;
		if(fetch_row(&dbi))
			order=(int)strtol(dbi.row[0],NULL,10)+1;
		else
			order=1;

		dbiClean(&dbi);
		sprintf(query,"INSERT INTO PlaylistSong(SongID,PlaylistID,\"Order\") VALUES(%d,%d,%d)",sid,pid,order);
		debug(3,query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

int getCategory(const char *arg){
	char query[401];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(CategoryID), CategoryID FROM Category WHERE Name=\'%s\'",arg);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create playlist
		dbiClean(&dbi);
		sprintf(query,"INSERT INTO Category (Name) VALUES ('%s')",arg);
		debug(3,query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

int getSongCategory(const int sid, const int cid){
	char query[201];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT COUNT(SongCatID), SongCatID FROM SongCategory WHERE CategoryID=%d AND SongID=%d",cid,sid);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create songcategory
		dbiClean(&dbi);
		sprintf(query,"INSERT INTO SongCategory(SongID,CategoryID) VALUES(%d,%d)",sid,cid);
		debug(3,query);
		if(sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

static int verifySong(const int sid){
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

static struct musicInfo *getMusicInfo(struct musicInfo *mi){
	static struct musicInfo *hold;
	if(mi)
		hold=mi;
	return hold;
}

#ifdef HAVE_FTW_H
#include <ftw.h>
int useFile(const char *fpath, const struct stat *sb, int typeflag) {
	if(typeflag==FTW_F)
		insertSong(fpath,NULL);
	return 0;
}

static int directoryInsert(const char *arg){
	if(ftw(arg,&useFile,1)){
		fprintf(stderr,"FTW Err\n");
		return 0;
	}
	return 1;
}
#else
static int directoryInsert(const char *arg){
	return insertSong(arg,NULL);
}
#endif

int batchInsert(char *arg){
	struct musicInfo mi;
	mi.title=malloc((MI_TITLE_SIZE*2+1)*sizeof(char));
	mi.track=malloc(((MI_TRACK_SIZE*2)+1)*sizeof(char));
	mi.artist=malloc(((MI_ARTIST_SIZE*2)+1)*sizeof(char));
	mi.album=malloc(((MI_ALBUM_SIZE*2)+1)*sizeof(char));
	mi.year=malloc(((MI_YEAR_SIZE*2)+1)*sizeof(char));
	mi.length=malloc(((MI_LENGTH_SIZE*2)+1)*sizeof(char));
	getMusicInfo(&mi);
	if(arg){//single argv insert
		directoryInsert(expand(arg));
	}
	else{//batch insert
		char temp[250];
		while(printf("File Location: ") && fgets(temp,sizeof(temp),stdin) && *temp!='\n'){
			expand(temp);
			insertSong(temp,&mi);
		}
	}
	miFree(&mi);
	return 1;
}

static unsigned int insertSong(const char *arg, struct musicInfo *mi){
	if(!mi)mi=getMusicInfo(NULL);
	miClean(mi);

	struct dbitem dbi;
	char dbq[350];
	char dbfilename[250];
	char library[200];
	char tempname[401];
	FILE *ffd;
	int x,songid=0,fmt,artistid,albumid;
	void *module;

	db_safe(dbfilename,arg,250);
	debug(1,arg);
	
	dbiInit(&dbi);
	//chack for dupicate
	sprintf(dbq,"SELECT SongID FROM Song WHERE Location='%s' LIMIT 1",dbfilename);
	doQuery(dbq,&dbi);
	if(dbi.row_count){
		debug(1,"duplicate entry -- skipping");
		dbiClean(&dbi);
		return 0;
	}

	debug(1,"Finding file type");
	if((fmt=fileFormat(arg))<1){
		dbiClean(&dbi);
		if(fmt==0)fprintf(stderr,"Unknown file type\n");
		return 0;
	}

	if((ffd=fopen(arg,"rb"))==NULL){
		dbiClean(&dbi);
		fprintf(stderr,"Can't open file\n");
		return 0;
	}

	function_meta modmeta;
	sprintf(dbq,"SELECT Library FROM FilePlugin WHERE TypeID=%d LIMIT 1",fmt);
	doQuery(dbq,&dbi);
	debug(3,dbq);
	while((x=getPlugin(&dbi,0,&module))){
		if(x>1)continue;
		modmeta=dlsym(module,"plugin_meta");
		if(!modmeta){
			debug(2,"Plugin does not contain plugin_meta().\n");
			continue;
		}
		else{
			modmeta(ffd,mi);
			break;
		}
		dlclose(module);
	}
	fclose(ffd);
	if(!x){
		dbiClean(&dbi);
		return 0;
	}

	dbiClean(&dbi);

	db_insert_safe(tempname,mi->artist,MI_ARTIST_SIZE);
	strcpy(mi->artist,tempname);
	if((artistid=getArtist(mi->artist))==-1){
		fprintf(stderr,"Error inserting artist.");
		return 0;
	}

	db_insert_safe(tempname,mi->album,MI_ALBUM_SIZE);
	strcpy(mi->album,tempname);
	if((albumid=getAlbum(mi->album,artistid))==-1){
		fprintf(stderr,"Error inserting album.");
		return 0;
	}
	if(strcmp(mi->album,"Unknown") && *mi->year){
		sprintf(dbq,"UPDATE Song SET TypeID=%d WHERE SongID=%d",fmt,songid);
		debug(3,dbq);
		sqlite3_exec(conn,dbq,NULL,NULL,NULL);
	}

	if(!*mi->title){
		strcpy(tempname,getFilename((char *)arg));
		db_insert_safe(mi->title,tempname,MI_TITLE_SIZE);
	}
	else{
		db_insert_safe(tempname,mi->title,MI_TITLE_SIZE);
		strcpy(mi->title,tempname);
	}
	if((songid=getSong(mi->title,dbfilename,albumid))==-1){
		fprintf(stderr,"Error inserting song.");
		return 0;
	}
	printf("%s | %s | %s \n",mi->title,mi->album,mi->artist);

	sprintf(dbq,"UPDATE Song SET Rating=3, TypeID=%d WHERE SongID=%d",fmt,songid);
	debug(3,dbq);
	sqlite3_exec(conn,dbq,NULL,NULL,NULL);

	getPlaylistSong(songid,getPlaylist("Library"));
	getSongCategory(songid,1); // Unknown
	debug(1,"Song inserted into Library");
	printf("\n");
	return 1;
}

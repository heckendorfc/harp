/*
 *  Copyright (C) 2009-2010  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "insert.h"
#include "defs.h"
#include "dbact.h"
#include "util.h"

static int verifySong(const int sid);
static int insertSong(const char *arg, struct musicInfo *mi);

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
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
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
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		sprintf(query,"INSERT INTO AlbumArtist (ArtistID,AlbumID) VALUES (%d,%d)",id,(int)sqlite3_last_insert_rowid(conn));
		debug(3,query);
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
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
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
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
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
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
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
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
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
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
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
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
		dbiClean(&dbi);
		if(!fgets(ans,sizeof(ans),stdin))return -1;
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

static struct pluginitem *getPluginList(struct pluginitem *list){
	static struct pluginitem *hold;
	if(list)
		hold=list;
	return hold;
}

#ifdef HAVE_FTW_H
#include <ftw.h>
int useFile(const char *fpath, const struct stat *sb, int typeflag) {
	if(typeflag==FTW_F)
		if(insertSong(fpath,NULL)<0)
			return -1; /* error */
	return 0; /* continue */
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
	struct pluginitem *plugin_head;
	
	if(!(plugin_head=openPlugins()))
		return 0;
	getPluginList(plugin_head);

	if(!(mi.title=malloc((MI_TITLE_SIZE*2+1)*sizeof(char))) ||
		!(mi.track=malloc(((MI_TRACK_SIZE*2)+1)*sizeof(char))) ||
		!(mi.artist=malloc(((MI_ARTIST_SIZE*2)+1)*sizeof(char))) ||
		!(mi.album=malloc(((MI_ALBUM_SIZE*2)+1)*sizeof(char))) ||
		!(mi.year=malloc(((MI_YEAR_SIZE*2)+1)*sizeof(char))))
		return 0;
	getMusicInfo(&mi);


	if(arg){//single argv insert
		if(isURL(arg))
			(void)insertSong(arg,&mi);
		else
			directoryInsert(expand(arg));
	}
	else{//batch insert
		char temp[250];
		while(printf("File Location: ") && fgets(temp,sizeof(temp),stdin) && *temp!='\n'){
			expand(temp);
			(void)insertSong(temp,&mi);
		}
	}
	miFree(&mi);
	return 1;
}

int metadataInsert(struct insert_data *data){
	int x;
	FILE *ffd;
	struct pluginitem *pi_ptr;

	findPluginIDByType(0); // Reset
	while((pi_ptr=findPluginByID(data->plugin_head,findPluginIDByType(data->fmt)))){
		if((ffd=pi_ptr->modopen(data->path,"rb"))!=NULL){
			debug(1,data->path);
			pi_ptr->modmeta(ffd,data->mi);
			pi_ptr->modclose(ffd);
			break;
		}
	}
	if(ffd==NULL)
		fprintf(stderr,"Can't open file\n");
	if(!pi_ptr){
		return -1; // Fatal error
	}
	return 1;
}

static void filepathinsert_flags(char flag, struct insert_data *data, int *limit, char **dest){
	switch(flag){
		case 'r':
			*limit=MI_ARTIST_SIZE;
			*dest=data->mi->artist;break;
		case 'a':
			*limit=MI_ALBUM_SIZE;
			*dest=data->mi->album;break;
		case 't':
			*limit=MI_TITLE_SIZE;
			*dest=data->mi->title;break;
		case 'y':
			*limit=MI_YEAR_SIZE;
			*dest=data->mi->year;break;
		case 'k':
			*limit=MI_TRACK_SIZE;
			*dest=data->mi->track;break;
	}
}

static void reverse_filepathInsert(char *orig_format, struct insert_data *data){
	char *format=orig_format;
	char *orig_ptr=(char*)data->path;
	char *ptr=orig_ptr;
	char *tmp;
	char *dest=NULL;
	int limit,x=0;
	while(*(++ptr)); // terminated
	while(*(++format)); // terminated
	while(format>orig_format && *(--format)!='%'); // Get first %
	while(format>=orig_format){
		filepathinsert_flags(*(format+1),data,&limit,&dest); // % coefficient
		if(dest && *(--format)!='%'){ // Char before %
			tmp=ptr;
			while((ptr--)>orig_ptr && *ptr!=*format); // Skip back to start of this str
				x=(int)(tmp-ptr);
			limit=x<limit?x:limit;
			memcpy(dest,ptr+1,limit);
			x=0;
			dest=NULL;
		}
		while(--format>orig_format && *format!='%' && (ptr--)>orig_ptr);
	}
	return;
}

int filepathInsert(struct insert_data *data){
	char **format=insertconf.format;
	char **f_root=insertconf.f_root;
	char *ptr=(char*)data->path;
	char *dest=NULL;
	char *fptr=*f_root;
	int limit,x=0;

	if(isURL(data->path)){
		strncpy(data->mi->title,data->path,MI_TITLE_SIZE);
		return 1;
	}

	while(fptr){
		if(*fptr=='*'){
			reverse_filepathInsert(*(format+x),data);
			return 1;
		}
		fptr=*(f_root+x);
		while(*ptr && *fptr && *(ptr++)==*(fptr++));
		if(!*fptr)break; // Success
		x++;
		ptr=(char*)data->path;
	}
	if(!fptr || !*(format+x))return 0;
	fptr=*(format+x);

	while(*fptr && *(fptr++)!='%');
	while(*fptr){
		filepathinsert_flags(*fptr,data,&limit,&dest);
		if(dest && *(++fptr)!='%'){
			while(*ptr!=*fptr && x++<limit)
				*(dest++)=*(ptr++);
			*dest=x=0;
			dest=NULL;
		}
		while(*fptr && *(fptr++)!='%' && *(ptr++));
	}
	return 1;
}

static int insertSong(const char *arg, struct musicInfo *mi){
	struct dbitem dbi;
	char query[350];
	char dbfilename[250];
	char tempname[401];
	FILE *ffd;
	unsigned int x,songid=0,artistid,albumid;
	int fmt;
	struct insert_data data;

	data.plugin_head=getPluginList(NULL);

	if(!mi)mi=getMusicInfo(NULL);
	miClean(mi);

	db_safe(dbfilename,arg,250);
	debug(1,arg);
	
	dbiInit(&dbi);
	//chack for dupicate
	sprintf(query,"SELECT SongID FROM Song WHERE Location='%s' LIMIT 1",dbfilename);
	harp_sqlite3_exec(conn,query,uint_return_cb,&songid,NULL);
	if(songid){
		debug(1,"Duplicate entry -- skipping");
		return 0;
	}

	debug(1,"Finding file type");
	if((fmt=fileFormat(data.plugin_head,arg))<1){
		if(fmt==0)fprintf(stderr,"Unknown file type\n");
		return 0;
	}

	data.fmt=fmt;
	data.query=query;
	data.mi=mi;
	data.dbi=&dbi;
	data.path=arg;

	if(insertconf.first_cb){
		x=insertconf.first_cb(&data);
		if(x<0)
			return -1;
		if(!x && insertconf.second_cb)
			insertconf.second_cb(&data);
	}
	dbiClean(&dbi);

	db_insert_safe(tempname,mi->artist,MI_ARTIST_SIZE);
	strcpy(mi->artist,tempname);
	if((artistid=getArtist(mi->artist))==-1){
		fprintf(stderr,"Error inserting artist: %s\n",mi->artist);
		return 0;
	}

	db_insert_safe(tempname,mi->album,MI_ALBUM_SIZE);
	strcpy(mi->album,tempname);
	if((albumid=getAlbum(mi->album,artistid))==-1){
		fprintf(stderr,"Error inserting album: %s\n",mi->album);
		return 0;
	}
	if(strcmp(mi->album,"Unknown") && *mi->year){
		sprintf(query,"UPDATE Album SET \"Date\"=%s WHERE AlbumID=%d",mi->year,albumid);
		debug(3,query);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
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
		fprintf(stderr,"Error inserting song: %s\n",mi->title);
		return 0;
	}

	// This prints escaped strings. Is it worth fixing?
	printf("%s | %s | %s \n",mi->title,mi->album,mi->artist);

	sprintf(query,"UPDATE Song SET Track=%d, Length=%d, Rating=3, TypeID=%d WHERE SongID=%d",(int)strtol(mi->track,NULL,10),mi->length,fmt,songid);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	getPlaylistSong(songid,getPlaylist("Library"));
	getSongCategory(songid,1); // Unknown
	debug(1,"Song inserted into Library");
	printf("\n");
	return 1;
}

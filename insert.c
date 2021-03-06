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

#include <regex.h>
#include "insert.h"
#include "defs.h"
#include "dbact.h"
#include "util.h"
#include "plugins/plugin.h"

//static int verifySong(const int sid);
static int insertSong(const char *arg, struct musicInfo *mi);

static void db_insert_safe(char *str, char *data, const size_t size){
	if(!*data){
		strlcpy(str,"Unknown",size);
		return;
	}
	db_clean(str,data,size);
	strlcpy(data,str,size);
	db_safe(str,data,size);
	if(!*str)strlcpy(str,"Unknown",size);
}

int getArtist(const char *arg){
	char query[301];
	struct dbitem dbi;
	dbiInit(&dbi);

	snprintf(query,301,"SELECT COUNT(ArtistID), ArtistID FROM Artist WHERE Name=\'%s\'",arg);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create artist
		dbiClean(&dbi);
		snprintf(query,301,"INSERT INTO Artist (Name) VALUES (\'%s\')",arg);
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

	snprintf(query,401,"SELECT COUNT(Album.AlbumID), Album.AlbumID FROM Album,AlbumArtist WHERE Album.Title=\'%s\' AND Album.AlbumID = AlbumArtist.AlbumID AND AlbumArtist.ArtistID=%d",arg,id);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create album
		dbiClean(&dbi);
		snprintf(query,401,"INSERT INTO Album (Title) VALUES (\'%s\')",arg);
		debug(3,query);
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		snprintf(query,401,"INSERT INTO AlbumArtist (ArtistID,AlbumID) VALUES (%d,%d)",id,(int)sqlite3_last_insert_rowid(conn));
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

	snprintf(query,401,"SELECT COUNT(Song.SongID), Song.SongID FROM Song, Album WHERE Song.Location=\'%s\' AND Song.AlbumID = Album.AlbumID AND Album.AlbumID=%d",loc,id);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create song
		dbiClean(&dbi);
		snprintf(query,401,"INSERT INTO Song (Title,Location,AlbumID,TypeID) VALUES (\'%s\',\'%s\',%d,%d)",arg,loc,id,-1);
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

	snprintf(query,401,"SELECT COUNT(Playlist.PlaylistID), Playlist.PlaylistID FROM Playlist WHERE Playlist.Title=\'%s\'",arg);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create playlist
		dbiClean(&dbi);
		snprintf(query,401,"INSERT INTO Playlist (Title) VALUES ('%s')",arg);
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

	snprintf(query,201,"SELECT COUNT(PlaylistSong.PlaylistSongID), PlaylistSong.PlaylistSongID FROM PlaylistSong WHERE PlaylistSong.PlaylistID=%d AND PlaylistSong.SongID=%d",pid,sid);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create playlistsong
		snprintf(query,201,"SELECT \"Order\" FROM PlaylistSong WHERE PlaylistID=%d ORDER BY \"Order\" DESC LIMIT 1",pid);
		debug(3,query);
		doQuery(query,&dbi);
		int order;
		if(fetch_row(&dbi))
			order=(int)strtol(dbi.row[0],NULL,10)+1;
		else
			order=1;

		dbiClean(&dbi);
		snprintf(query,201,"INSERT INTO PlaylistSong(SongID,PlaylistID,\"Order\") VALUES(%d,%d,%d)",sid,pid,order);
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

	snprintf(query,401,"SELECT COUNT(CategoryID), CategoryID FROM Category WHERE Name=\'%s\'",arg);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create playlist
		dbiClean(&dbi);
		snprintf(query,401,"INSERT INTO Category (Name) VALUES ('%s')",arg);
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

	snprintf(query,201,"SELECT COUNT(SongCatID), SongCatID FROM SongCategory WHERE CategoryID=%d AND SongID=%d",cid,sid);
	debug(3,query);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(strtol(dbi.row[0],NULL,10)==0){//create songcategory
		dbiClean(&dbi);
		snprintf(query,201,"INSERT INTO SongCategory(SongID,CategoryID) VALUES(%d,%d)",sid,cid);
		debug(3,query);
		if(harp_sqlite3_exec(conn,query,NULL,NULL,NULL)!=SQLITE_OK)
			return -1;
		return sqlite3_last_insert_rowid(conn);
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

/*
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
*/

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
	char temp[250];

	if(!(mi.title=malloc((MI_TITLE_SIZE*2+1)*sizeof(char))) ||
		!(mi.track=malloc(((MI_TRACK_SIZE*2)+1)*sizeof(char))) ||
		!(mi.artist=malloc(((MI_ARTIST_SIZE*2)+1)*sizeof(char))) ||
		!(mi.album=malloc(((MI_ALBUM_SIZE*2)+1)*sizeof(char))) ||
		!(mi.year=malloc(((MI_YEAR_SIZE*2)+1)*sizeof(char))))
		return 0;
	getMusicInfo(&mi);

	if(arg){//single argv insert
		if(isURL(arg)){
			(void)insertSong(arg,&mi);
		}
		else{
			strlcpy(temp,arg,250);
			directoryInsert(expand(temp,250));
		}
	}
	else{//batch insert
		while(printf("File Location: ") && fgets(temp,sizeof(temp),stdin) && *temp!='\n'){
			expand(temp,250);
			(void)insertSong(temp,&mi);
		}
	}
	miFree(&mi);
	return 1;
}

int metadataInsert(struct insert_data *data){
	FILE *ffd=NULL;
	struct pluginitem *pi_ptr=NULL;

	//while((pi_ptr=findPluginByID(plugin_head,findPluginIDByType(data->fmt)))){
	if(data->fmt<PLUGIN_NULL && (pi_ptr=plugin_head[data->fmt])){
		if((ffd=pi_ptr->modopen(data->path,"rb"))!=NULL){
			debug(1,data->path);
			pi_ptr->modmeta(ffd,data->mi);
			pi_ptr->modclose(ffd);
		}
	}
	if(ffd==NULL)
		fprintf(stderr,"Can't open file\n");
	if(!pi_ptr){
		return -1; // Fatal error
	}
	return 1;
}

void setInsertData(struct musicInfo *mi, char *str, int start, int end, int id){
	int len=end-start;
	char *ptr;

	switch(id){
		case MI_ARTIST:
			ptr=mi->artist;
			break;
		case MI_ALBUM:
			ptr=mi->album;
			break;
		case MI_TITLE:
			ptr=mi->title;
			break;
		case MI_TRACK:
			ptr=mi->track;
			break;
		case MI_YEAR:
			ptr=mi->year;
			break;
		default:
			ptr=NULL;
	}
	if(ptr)
		memcpy(ptr,str+start,len);
}

int filepathInsert(struct insert_data *data){
	char *ptr=(char*)data->path;
	const size_t nrm=9;
	int **ko=insertconf.keyorder;
	regex_t *preg=insertconf.pattern;
	regmatch_t rmatch[nrm];
	int i,j;
	int rret;

	for(i=0;ko[i];i++){
		rret=regexec(preg+i,ptr,nrm,rmatch,0);
		if(rret==0){
			for(j=0;ko[i][j]<MI_NULL && j<nrm-1;j++){
				setInsertData(data->mi,ptr,rmatch[j+1].rm_so,rmatch[j+1].rm_eo,ko[i][j]);
			}
			return 1;
		}
		else if(rret==REG_NOMATCH){
		}
		else{
			debug(1,"Regex match error!");
		}
	}

	debug(1,"No regex matches.");
	return 1;
}

static int insertSong(const char *arg, struct musicInfo *mi){
	struct dbitem dbi;
	char query[350];
	char dbfilename[250];
	char tempname[401];
	unsigned int songid=0,artistid,albumid;
	int fmt;
	int x;
	struct insert_data data;

	//data.plugin_head=getPluginList(NULL);

	if(!mi)mi=getMusicInfo(NULL);
	miClean(mi);

	db_safe(dbfilename,arg,250);
	debug(1,arg);

	dbiInit(&dbi);
	//chack for dupicate
	snprintf(query,350,"SELECT SongID FROM Song WHERE Location='%s' LIMIT 1",dbfilename);
	harp_sqlite3_exec(conn,query,uint_return_cb,&songid,NULL);
	if(songid){
		debug(1,"Duplicate entry -- skipping");
		return 0;
	}

	debug(1,"Finding file type");
	if((fmt=fileFormat(plugin_head,arg))>=PLUGIN_NULL){
		if(fmt==PLUGIN_NULL)
			fprintf(stderr,"Unknown file type\n");
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
	strlcpy(mi->artist,tempname,MI_ARTIST_SIZE);
	if((artistid=getArtist(mi->artist))==-1){
		fprintf(stderr,"Error inserting artist: %s\n",mi->artist);
		return 0;
	}

	db_insert_safe(tempname,mi->album,MI_ALBUM_SIZE);
	strlcpy(mi->album,tempname,MI_ALBUM_SIZE);
	if((albumid=getAlbum(mi->album,artistid))==-1){
		fprintf(stderr,"Error inserting album: %s\n",mi->album);
		return 0;
	}
	if(strcmp(mi->album,"Unknown") && *mi->year){
		snprintf(query,350,"UPDATE Album SET \"Date\"=%s WHERE AlbumID=%d",mi->year,albumid);
		debug(3,query);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}

	if(!*mi->title){
		strlcpy(tempname,getFilename((char *)arg),MI_TITLE_SIZE);
		db_insert_safe(mi->title,tempname,MI_TITLE_SIZE);
	}
	else{
		db_insert_safe(tempname,mi->title,MI_TITLE_SIZE);
		strlcpy(mi->title,tempname,MI_TITLE_SIZE);
	}
	if((songid=getSong(mi->title,dbfilename,albumid))==-1){
		fprintf(stderr,"Error inserting song: %s\n",mi->title);
		return 0;
	}

	// This prints escaped strings. Is it worth fixing?
	printf("%s | %s | %s \n",mi->title,mi->album,mi->artist);

	snprintf(query,350,"UPDATE Song SET Track=%d, Length=%d, Rating=3, TypeID=%d WHERE SongID=%d",(int)strtol(mi->track,NULL,10),mi->length,fmt,songid);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	getPlaylistSong(songid,getPlaylist("Library"));
	getSongCategory(songid,1); // Unknown
	debug(1,"Song inserted into Library");
	printf("\n");
	return 1;
}

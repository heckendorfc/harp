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

#include "edit.h"
#include "defs.h"
#include "dbact.h"
#include "util.h"
#include "portal.h"
#include "tree.h"
#include "insert.h"
#include "list.h"

static void cleanOrphans();

static int editSongName(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	char query[200];
	int x;

	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Name: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	if(songids->length>1 && !editWarn("This operation will alter multiple songs.")){
			printf("Aborted\n");
			return PORTAL_RET_PREV;
	}

	for(x=0;x<songids->length;x++){
		snprintf(query,200,"UPDATE Song SET Title='%s' WHERE SongID=%d",args,songids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return PORTAL_RET_PREV;
}

static int editSongLocation(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	char query[200];
	int x;

	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Location: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	if(songids->length>1 && !editWarn("This operation will alter multiple songs.")){
			printf("Aborted\n");
			return PORTAL_RET_PREV;
	}

	for(x=0;x<songids->length;x++){
		snprintf(query,200,"UPDATE Song SET Location='%s' WHERE SongID=%d",args,songids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return PORTAL_RET_PREV;
}

static int editSongArtist(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	char query[200],*ptr;
	int x,albumid,artistid;
	struct dbitem dbi;

	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	dbiInit(&dbi);

	if((x=getStdArgs(args,"Artist: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	if((artistid=(int)strtol(args,NULL,10))<1)
		artistid=getArtist(args);
	snprintf(query,200,"SELECT Album.Title FROM Album,Song WHERE Album.AlbumID=Song.AlbumID AND SongID=%d",songids->songid[0]);
	debug(3,query);
	if(doQuery(query,&dbi)>0 && fetch_row(&dbi))
		albumid=getAlbum(dbi.row[0],artistid);
	else{
		debug(2,"getAlbum error in editSongArtist");
		dbiClean(&dbi);
		return PORTAL_RET_PREV;
	}

	snprintf(query,200,"UPDATE Song SET AlbumID=%d WHERE SongID=",albumid);
	debug(3,query);
	for(x=0;query[x];x++);
	ptr=&query[x];
	for(x=0;x<songids->length;x++){
		snprintf(ptr,200-(ptr-query),"%d",songids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	dbiClean(&dbi);
	return PORTAL_RET_PREV;
}

static int editSongAlbum(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	char query[200],*ptr;
	int x,albumid,artistid;
	struct dbitem dbi;

	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	dbiInit(&dbi);

	if((x=getStdArgs(args,"Album title: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	snprintf(query,200,"SELECT ArtistID from Song NATURAL JOIN AlbumArtist WHERE SongID=%d",songids->songid[0]);
	debug(3,query);
	if(doQuery(query,&dbi)<1 || !fetch_row(&dbi) || (artistid=(int)strtol(dbi.row[0],NULL,10))<1){
		debug(2,"getArtist error in getAlbum");
		return PORTAL_RET_PREV;
	}

	albumid=getAlbum(args,artistid);

	snprintf(query,200,"UPDATE Song SET AlbumID=%d WHERE SongID=",albumid);
	debug(3,query);
	for(x=0;query[x];x++);
	ptr=&query[x];
	for(x=0;x<songids->length;x++){
		snprintf(ptr,200-(ptr-query),"%d",songids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	dbiClean(&dbi);
	return PORTAL_RET_PREV;
}

static int deleteSong(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	char query[100];
	int x;

	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}
	printf("Delete?: ");
	if(!fgets(args,PORTAL_ARG_LEN,stdin))return PORTAL_RET_PREV;
	if(*args=='y' || *args=='Y'){
		for(x=0;x<songids->length;x++){
			snprintf(query,100,"DELETE FROM Song WHERE SongID=%d",songids->songid[x]);
			harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
			snprintf(query,100,"DELETE FROM PlaylistSong WHERE SongID=%d",songids->songid[x]);
			harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		debug(1,"Songs deleted.");
		return PORTAL_RET_MAIN;
	}
	return PORTAL_RET_PREV;
}

static int editSongTrack(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	char query[100],*ptr;
	int x;

	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Track number: "))<0)return PORTAL_RET_PREV;

	snprintf(query,100,"UPDATE Song SET Track=%d WHERE SongID=",(int)strtol(&args[x],NULL,10));
	for(x=0;query[x];x++);
	ptr=query+x;

	for(x=0;x<songids->length;x++){
		snprintf(ptr,100-(ptr-query),"%d",songids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return PORTAL_RET_PREV;
}

static int songActivation(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	char query[100],*ptr;
	int x;

	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}
	for(x=1;x<PORTAL_ARG_LEN && args[x] && args[x]==' ';x++);
	if(args[x] && args[x]>='0' && args[x]<='9'){
		snprintf(query,100,"UPDATE Song SET Active=%d WHERE SongID=",(strtol(&args[x],NULL,10)>0?1:0));
		ptr=&query[38];
	}
	else{
		snprintf(query,100,"UPDATE Song SET Active=NOT(Active) WHERE SongID=");
		ptr=&query[48];
	}
	debug(3,query);
	for(x=0;x<songids->length;x++){
		snprintf(ptr,100-(ptr-query),"%d",songids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return PORTAL_RET_PREV;
}

static int editAlbumArtist(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200],*ptr;
	int x,artistid;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Artist: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	artistid=getArtist(args);
	snprintf(query,200,"UPDATE AlbumArtist SET ArtistID=%d WHERE AlbumID=",artistid);
	for(x=0;query[x];x++);
	ptr=&query[x];
	debug(3,query);
	for(x=0;x<ids->length;x++){
		snprintf(ptr,200-(ptr-query),"%d",ids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	cleanOrphans();
	return PORTAL_RET_PREV;
}

static int editAlbumTitle(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x,albumid,artistid;
	struct dbitem dbi;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	dbiInit(&dbi);

	if((x=getStdArgs(args,"Album title: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	snprintf(query,200,"SELECT ArtistID from AlbumArtist WHERE AlbumID=%d",ids->songid[0]);
	debug(3,query);
	if(doQuery(query,&dbi)<1 || !fetch_row(&dbi) || (artistid=(int)strtol(dbi.row[0],NULL,10))<1){
		debug(2,"getArtist error in editAlbumTitle");
		return PORTAL_RET_PREV;
	}

	albumid=getAlbum(args,artistid);
	for(x=0;x<ids->length;x++){
		snprintf(query,200,"UPDATE Song SET AlbumID=%d WHERE AlbumID=%d",albumid,ids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		// The following queries will also be taken care of by cleanOrphans. Leave it for now.
		snprintf(query,200,"DELETE FROM AlbumArtist WHERE AlbumID=%d",ids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		if(albumid!=ids->songid[x]){
			snprintf(query,200,"DELETE FROM Album WHERE AlbumID=%d",ids->songid[x]);
			harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		ids->songid[x]=albumid;
	}
	ids->length=1;
	dbiClean(&dbi);
	return PORTAL_RET_PREV;
}

static int editAlbumDate(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x,date;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Album date: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	date=(int)strtol(args,NULL,10);

	for(x=0;x<ids->length;x++){
		snprintf(query,200,"UPDATE Album SET \"Date\"=%d WHERE AlbumID=%d",date,ids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return PORTAL_RET_PREV;
}

static int editArtistName(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x,artistid;
	struct dbitem dbi;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	dbiInit(&dbi);

	if((x=getStdArgs(args,"Artist: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	artistid=getArtist(args);
	for(x=0;x<ids->length;x++){
		snprintf(query,200,"UPDATE AlbumArtist SET ArtistID=%d WHERE ArtistID=%d",artistid,ids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		if(ids->songid[x]!=artistid){
			snprintf(query,200,"DELETE FROM Artist WHERE ArtistID=%d",ids->songid[x]);
			harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		ids->songid[x]=artistid;
	}
	ids->length=1;
	dbiClean(&dbi);
	return PORTAL_RET_PREV;
}

static int editPlaylistName(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Name: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	if(ids->length>1 && !editWarn("This operation will alter only the first playlist in the list.")){
			printf("Aborted\n");
			return PORTAL_RET_PREV;
	}

	snprintf(query,200,"UPDATE Playlist SET Title='%s' WHERE PlaylistID=%d",args,ids->songid[0]);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return PORTAL_RET_PREV;
}

static int deletePlaylist(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	printf("Delete playlist?: ");
	if(!fgets(args,PORTAL_ARG_LEN,stdin))return PORTAL_RET_PREV;
	if(*args=='y' || *args=='Y'){
		for(x=0;x<ids->length;x++){
			snprintf(query,200,"DELETE FROM PlaylistSong WHERE PlaylistID=%d",ids->songid[x]);
			harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
			if(ids->songid[x]!=1){ // If Library: delete playlistsongs but leave playlist
				snprintf(query,200,"DELETE FROM Playlist WHERE PlaylistID=%d",ids->songid[x]);
				harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
			}
		}
		return PORTAL_RET_MAIN;
	}
	return PORTAL_RET_PREV;
}

static int editPlaylistSongAdd(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	struct dbitem dbi;
	int x,order,songid;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	dbiInit(&dbi);

	if((x=getStdArgs(args,"Song: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	*arglist[ATYPE].subarg='s';
	if((songid=getID(args))<1){
		fprintf(stderr,"No song found.\n");
		return PORTAL_RET_PREV;
	}
	*arglist[ATYPE].subarg='p';

	for(x=0;x<ids->length;x++){
		snprintf(query,200,"SELECT \"Order\" FROM PlaylistSong WHERE PlaylistID=%d ORDER BY \"Order\" DESC LIMIT 1",ids->songid[x]);
		doQuery(query,&dbi);
		if(fetch_row(&dbi))
			order=(int)strtol(dbi.row[0],NULL,10)+1;
		else
			order=1;

		snprintf(query,200,"INSERT INTO PlaylistSong(PlaylistID,SongID,\"Order\") VALUES (%d,%d,%d)",ids->songid[x],songid,order);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	dbiClean(&dbi);
	return PORTAL_RET_PREV;
}

static int editPlaylistSongDelete(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Song order: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	if(ids->length>1 && !editWarn("This operation will alter only the first playlist in the list.")){
			printf("Aborted\n");
			return PORTAL_RET_PREV;
	}

	x=(int)strtol(args,NULL,10);
	snprintf(query,200,"DELETE FROM PlaylistSong WHERE \"Order\"=%d AND PlaylistID=%d",x,ids->songid[0]);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	// Remove the empty place at the old order
	snprintf(query,200,"UPDATE PlaylistSong SET \"Order\"=\"Order\"-1 WHERE \"Order\">%d AND PlaylistID=%d",x,ids->songid[0]);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return PORTAL_RET_PREV;
}

static void getPlaylistSongOrders(char *args, int *cur, int*new){
	int x;
	*cur=*new=0;
	for(x=1;args[x] && (args[x]<'0' || args[x]>'9');x++);
	if(!args[x]){
		printf("Current order: ");
		if(!fgets(args,PORTAL_ARG_LEN,stdin) || *args=='\n'){
			printf("Aborted\n");
			*cur=*new=0;
			return;
		}
		cleanString(args);
		*cur=(int)strtol(args,NULL,10);

		printf("New order: ");
		if(!fgets(args,PORTAL_ARG_LEN,stdin) || *args=='\n'){
			printf("Aborted\n");
			*cur=*new=0;
			return;
		}
		cleanString(args);
		*new=(int)strtol(args,NULL,10);
	}
	else{
		*cur=(int)strtol(&args[x],NULL,10);

		// Find the end of the current order
		for(;args[x] && args[x]>='0' && args[x]<='9';x++);
		// Find the start of the new order
		for(;args[x] && (args[x]<'0' || args[x]>'9');x++);
		if(!args[x]){
			printf("New order: ");
			if(!fgets(args,PORTAL_ARG_LEN,stdin) || *args=='\n'){
				printf("Aborted\n");
				*cur=*new=0;
				return;
			}
			cleanString(args);
			x=0;
		}
		*new=(int)strtol(&args[x],NULL,10);
	}
}

static int editPlaylistSongOrder(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int current_order,new_order;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if(ids->length>1 && !editWarn("This operation will alter only the first playlist in the list.")){
			printf("Aborted\n");
			return PORTAL_RET_PREV;
	}

	getPlaylistSongOrders(args,&current_order,&new_order);
	if(current_order<1 || new_order<1){
		printf("Invalid order\n");
		return PORTAL_RET_PREV;
	}

	// Move song out of the way. Order 0 should not be used.
	snprintf(query,200,"UPDATE PlaylistSong SET \"Order\"=0 WHERE PlaylistID=%d AND \"Order\"=%d",ids->songid[0],current_order);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	// Remove the empty place at the old order
	snprintf(query,200,"UPDATE PlaylistSong SET \"Order\"=\"Order\"-1 WHERE \"Order\">%d AND PlaylistID=%d",current_order,ids->songid[0]);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	// Make room for the song. TODO: find performance of ignoring the overlap vs testing for overlap in the query.
	snprintf(query,200,"UPDATE PlaylistSong SET \"Order\"=\"Order\"+1 WHERE \"Order\">%d AND PlaylistID=%d",new_order-1,ids->songid[0]);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	// Change the song's order
	snprintf(query,200,"UPDATE PlaylistSong SET \"Order\"=%d WHERE PlaylistID=%d AND \"Order\"=0",new_order,ids->songid[0]);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return PORTAL_RET_PREV;
}

static int editPlaylistCreate(char *args, void *data){
	char *ptr;
	int x,pid,sid;

	if((x=getStdArgs(args,"Playlist name: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	pid=getPlaylist(args);
	if(pid<1){
		fprintf(stderr,"Error inserting playlist\n");
		return PORTAL_RET_PREV;
	}
	*arglist[ATYPE].subarg='s';
	while(printf("SongID: ") && fgets(args,PORTAL_ARG_LEN,stdin) && *args!='\n'){
		ptr=args;
		while(*(++ptr)!='\n');
		*ptr=0;
		if((sid=getID(args))<1)continue;
		//if((sid=verifySong(sid))>0) // I don't know if I like this or not. Disabled for now.
		getPlaylistSong(sid,pid);
	}
	return PORTAL_RET_MAIN;
}

static int editGenreCreate(char *args, void *data){
	int x,gid;

	if((x=getStdArgs(args,"Genre name: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	gid=getCategory(args);
	if(gid<1){
		fprintf(stderr,"Error inserting genre\n");
		return PORTAL_RET_PREV;
	}
	return PORTAL_RET_MAIN;
}

static int editGenreName(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Genre name: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	for(x=0;x<ids->length;x++){
		snprintf(query,200,"UPDATE Category SET Name='%s' WHERE CategoryID=%d",args,ids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return PORTAL_RET_PREV;
}

static int editGenreParent(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x,gid;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Owner: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	*arglist[ATYPE].subarg='g';
	if((gid=getID(args))<1){
		if(!strncasecmp(args,"none",4) || *args=='0')
			gid=0;
		else
			return PORTAL_RET_PREV;
	}

	for(x=0;x<ids->length;x++){
		if(gid==ids->songid[x])continue;
		snprintf(query,200,"UPDATE Category SET ParentID=%d WHERE CategoryID=%d AND CategoryID NOT IN (SELECT ParentID FROM Category WHERE CategoryID=%d)",gid,ids->songid[x],gid);
		debug(3,query);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return PORTAL_RET_PREV;
}

static int editGenreDelete(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}
	if(!editWarn("Delete genre?"))return PORTAL_RET_MAIN;

	for(x=0;x<ids->length;x++){
		if(ids->songid[x]==1)continue;

		snprintf(query,200,"UPDATE Category SET ParentID=(SELECT ParentID FROM Category WHERE CategoryID=%d) WHERE ParentID=%d",ids->songid[x],ids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		snprintf(query,200,"DELETE FROM Category WHERE CategoryID=%d",ids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		snprintf(query,200,"DELETE FROM SongCategory WHERE CategoryID=%d",ids->songid[x]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return PORTAL_RET_MAIN;
}

static int editSongGenreAdd(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[300];
	int x,gid;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Genre to add: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	*arglist[ATYPE].subarg='g';
	if((gid=getID(args))<1){
		fprintf(stderr,"Invalid genre\n");
		return PORTAL_RET_PREV;
	}

	// Insert songs from list that are not already in the category
	snprintf(query,300,"INSERT INTO SongCategory(CategoryID,SongID) SELECT '%d',SongID FROM SongCategory WHERE SongID NOT IN (SELECT SongID FROM SongCategory WHERE CategoryID=%d) AND SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",gid,gid,ids->tempselectid);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	return PORTAL_RET_PREV;
}

static int editSongGenreRemove(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x,gid;

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	if((x=getStdArgs(args,"Genre to remove: "))<0)return PORTAL_RET_PREV;
	args=&args[x];

	*arglist[ATYPE].subarg='g';
	if((gid=getID(args))<1){
		fprintf(stderr,"Invalid genre\n");
		return PORTAL_RET_PREV;
	}

	snprintf(query,200,"DELETE FROM SongCategory WHERE CategoryID=%d AND SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",gid,ids->tempselectid);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	return PORTAL_RET_PREV;
}

static void cleanOrphans(){
	// Clean orphaned albums
	harp_sqlite3_exec(conn,"DELETE FROM Album WHERE AlbumID NOT IN (SELECT AlbumID FROM Song)",NULL,NULL,NULL);
	// Clean orphaned albumartists
	harp_sqlite3_exec(conn,"DELETE FROM AlbumArtist WHERE AlbumID NOT IN (SELECT AlbumID FROM Song)",NULL,NULL,NULL);
	// Clean orphaned artists
	harp_sqlite3_exec(conn,"DELETE FROM Artist WHERE ArtistID NOT IN (SELECT ArtistID FROM AlbumArtist)",NULL,NULL,NULL);
	// Put orphaned [category] songs in unknown
	harp_sqlite3_exec(conn,"INSERT INTO SongCategory (SongID,CategoryID) SELECT SongID,'1' FROM Song WHERE SongID NOT IN (SELECT SongID FROM SongCategory)",NULL,NULL,NULL);
}

static int listSongs(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	char query[200];
	int x=0;
	int exception[10];

	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	for(x=0;x<10;x++)exception[x]=x<5?1:listconf.exception;
	snprintf(query,200,"SELECT SongID, SongTitle, SongTrack, Location, AlbumTitle AS Album, ArtistName AS Artist FROM SongPubInfo WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",songids->tempselectid);
	doTitleQuery(query,exception,listconf.maxwidth);
	return PORTAL_RET_PREV;
}

static int listAlbums(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x=0;
	int exception[10];

	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	for(x=0;x<10;x++)exception[x]=x<1?1:listconf.exception;
	snprintf(query,200,"SELECT AlbumID, Title, \"Date\" FROM Album WHERE AlbumID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",ids->tempselectid);
	doTitleQuery(query,exception,listconf.maxwidth);
	return PORTAL_RET_PREV;
}

static int listArtists(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	char query[200];
	int x=0;
	int exception[10];
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return PORTAL_RET_PREV;
	}

	for(x=0;x<10;x++)exception[x]=x<1?1:listconf.exception;
	snprintf(query,200,"SELECT ArtistID, Name FROM Artist WHERE ArtistID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",ids->tempselectid);
	doTitleQuery(query,exception,listconf.maxwidth);
	return PORTAL_RET_PREV;
}

static int listPlaylists(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	struct dbitem dbi;
	char query[120];
	int x=0;
	int exception[10];

	if(!ids || !ids->songid){
		*arglist[ATYPE].subarg='p';
		listall();
		fprintf(stderr,"None selected.\n");
		return PORTAL_RET_PREV;
	}

	dbiInit(&dbi);

	// List contents of playlist as well.
	if(args[1]=='C'){
		*arglist[ATYPE].subarg='p';
		list(ids->songid,ids->length);
		return PORTAL_RET_PREV;
	}

	for(x=0;x<10;x++)exception[x]=x<1?1:listconf.exception;
	snprintf(query,120,"SELECT PlaylistID, Title FROM Playlist WHERE PlaylistID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",ids->tempselectid);
	doTitleQuery(query,exception,listconf.maxwidth);
	return PORTAL_RET_PREV;
}

static int listGenre(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	int x=0;

	if(!ids || !ids->songid){
		printGenreTree(0,(void *)tierCatPrint);
		fprintf(stderr,"None selected.\n");
		return PORTAL_RET_PREV;
	}
	// List contents of genre as well.
	if(args[1]=='C'){
		for(x=0;x<ids->length;x++){
			printGenreTree(ids->songid[x],(void *)tierChildPrint);
		}
	}
	else{
		for(x=0;x<ids->length;x++){
			printGenreTree(ids->songid[x],(void *)tierCatPrint);
		}
	}
	return PORTAL_RET_PREV;
}

static int listSongGenre(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	int x=0;
	char query[221];
	int exception[10];
	if(!ids || !ids->songid){
		fprintf(stderr,"None selected.\n");
		return PORTAL_RET_PREV;
	}
	for(x=2;x<10;x++)exception[x]=listconf.exception;
	exception[0]=exception[1]=1;

	// Print a list of genres that the songs belong to.
	snprintf(query,221,"SELECT CategoryID AS ID,Name FROM Category WHERE ID IN (SELECT DISTINCT CategoryID FROM SongCategory WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d))",ids->tempselectid);
	debug(3,query);
	doTitleQuery(query,exception,listconf.maxwidth);
	return PORTAL_RET_PREV;
}

static int songGenrePortal(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	struct commandOption portalOptions[]={
		{'L',listSongGenre,"List genres",ids},
		{'a',editSongGenreAdd,"Add songs to a genre",ids},
		{'r',editSongGenreRemove,"Remove songs from a genre",ids},
		{0,NULL,NULL,NULL}
	};

	if(!ids || !ids->songid){
		fprintf(stderr,"None selected.\n");
		return PORTAL_RET_PREV;
	}
	return portal(portalOptions,"Song-Genre");
}

static int songPortal(char *args, void *data){
	int x,*song_ids=malloc(sizeof(int));
	struct dbitem dbi;
	struct IDList sid_struct;
	struct IDList *id_struct=&sid_struct;
	struct commandOption portalOptions[]={
		{'L',listSongs,"List affected songs",id_struct},
		{'t',editSongName,"Change title",id_struct},
		{'k',editSongTrack,"Change track number",id_struct},
		{'l',editSongLocation,"Change location",id_struct},
		{'a',editSongAlbum,"Change album",id_struct},
		{'r',editSongArtist,"Change artist",id_struct},
		{'d',deleteSong,"Delete from database",id_struct},
		{'v',songActivation,"Toggle activation",id_struct},
		{'g',songGenrePortal,"Manage genre",id_struct},
		{0,NULL,NULL,NULL}
	};
	dbiInit(&dbi);


	for(x=1;x<PORTAL_ARG_LEN && args[x] && args[x]<'0';x++);
	if(!args[x]){
		fprintf(stderr,"Required argument not given.\n");
		return PORTAL_RET_PREV;
	}

	if(getGroupSongIDs(args+x,PORTAL_ARG_LEN,id_struct)==HARP_RET_ERR)
		return PORTAL_RET_PREV;

	x=portal(portalOptions,"Song");
	cleanTempSelect(id_struct->tempselectid);
	free(song_ids);
	return x;
}

static int albumPortal(char *args, void *data){
	struct IDList ids_struct;
	struct IDList *ids=&ids_struct;
	struct commandOption portalOptions[]={
		{'L',listAlbums,"List affected alubms",ids},
		{'t',editAlbumTitle,"Change title",ids},
		{'r',editAlbumArtist,"Change artist",ids},
		{'d',editAlbumDate,"Change date",ids},
		{0,NULL,NULL,NULL}
	};
	int x;

	for(x=1;x<PORTAL_ARG_LEN && args[x] && args[x]<'0';x++);
	if(!args[x]){
		fprintf(stderr,"Required argument not given.\n");
		return PORTAL_RET_PREV;
	}

	arglist[ATYPE].subarg[0]='a';

	if(!(ids->songid=getMulti(&args[x],&ids->length)))
		return PORTAL_RET_PREV;
	if(ids->songid[0]<1){
		fprintf(stderr,"No results found.\n");
		free(ids->songid);
		return PORTAL_RET_PREV;
	}
	ids->tempselectid=insertTempSelect(ids->songid,ids->length);

	x=portal(portalOptions,"Album");
	cleanTempSelect(ids->tempselectid);
	free(ids->songid);
	return x;
}

static int artistPortal(char *args, void *data){
	struct IDList ids_struct;
	struct IDList *ids=&ids_struct;
	struct commandOption portalOptions[]={
		{'L',listArtists,"List affected artists",ids},
		{'n',editArtistName,"Change name",ids},
		{0,NULL,NULL,NULL}
	};
	int x;

	for(x=1;x<PORTAL_ARG_LEN && args[x] && args[x]<'0';x++);
	if(!args[x]){
		fprintf(stderr,"Required argument not given.\n");
		return PORTAL_RET_PREV;
	}

	arglist[ATYPE].subarg[0]='r';

	if(!(ids->songid=getMulti(&args[x],&ids->length)))
		return PORTAL_RET_PREV;
	if(ids->songid[0]<1){
		fprintf(stderr,"No results found.\n");
		return PORTAL_RET_PREV;
	}
	ids->tempselectid=insertTempSelect(ids->songid,ids->length);

	x=portal(portalOptions,"Artist");
	cleanTempSelect(ids->tempselectid);
	free(ids->songid);
	return x;
}

static int playlistPortal(char *args, void *data){
	struct IDList ids_struct;
	struct IDList *ids=&ids_struct;
	int x;

	for(x=1;x<PORTAL_ARG_LEN && args[x] && args[x]<'0';x++);
	if(args[x]){
		struct commandOption portalOptions[]={
			{'L',listPlaylists,"List affected playlist\nLC\tList the contents of the selected playlist",ids},
			{'c',editPlaylistCreate,"Create a playlist",NULL},
			{'n',editPlaylistName,"Change name",ids},
			{'d',deletePlaylist,"Delete playlist from database",ids},
			{'a',editPlaylistSongAdd,"Add a song",ids},
			{'r',editPlaylistSongDelete,"Remove a song from playlist",ids},
			{'o',editPlaylistSongOrder,"Change the order of a song",ids},
			{0,NULL,NULL,NULL}
		};

		arglist[ATYPE].subarg[0]='p';

		if(!(ids->songid=getMulti(&args[x],&ids->length)))
			return PORTAL_RET_PREV;
		if(ids->songid[0]<1){
			fprintf(stderr,"No results found.\n");
			return PORTAL_RET_PREV;
		}
		for(x=0;x<ids->length;x++){
			if(ids->songid[x]==1){
				fprintf(stderr,"You may not edit the Library\n");
				free(ids->songid);
				return PORTAL_RET_PREV;
			}
		}
		ids->tempselectid=insertTempSelect(ids->songid,ids->length);
		x=portal(portalOptions,"Playlist");
		cleanTempSelect(ids->tempselectid);
		free(ids->songid);
		return x;
	}
	else{
		struct commandOption portalOptions[]={
			{'L',listPlaylists,"Create a playlist",NULL},
			{'c',editPlaylistCreate,"Create a playlist",NULL},
			{0,NULL,NULL,NULL}
		};
		fprintf(stderr,"No playlist selected.\n");
		return portal(portalOptions,"Playlist");
	}
}

static int genrePortal(char *args, void *data){
	struct IDList ids_struct;
	struct IDList *ids=&ids_struct;
	struct commandOption portalOptions[]={
		{'L',listGenre,"List affected genre\nLC\tList the contents of the selected genre",ids},
		{'n',editGenreName,"Change name of selected genre",ids},
		{'o',editGenreParent,"Change the owner of the genre",ids},
		{'d',editGenreDelete,"Delete genre from database",ids},
		{'a',editGenreCreate,"Add a new genre",NULL},
		{0,NULL,NULL,NULL}
	};
	int x;

	for(x=1;x<PORTAL_ARG_LEN && args[x] && args[x]<'0';x++);
	if(!args[x]){
		fprintf(stderr,"No genre selected.\n");
		struct commandOption portalOptions[]={
			{'L',listGenre,"List all genres",NULL},
			{'a',editGenreCreate,"Add a new genre",NULL},
			{0,NULL,NULL,NULL}
		};
		return portal(portalOptions,"Genre");
	}

	arglist[ATYPE].subarg[0]='g';

	if(!(ids->songid=getMulti(&args[x],&ids->length)))
		return PORTAL_RET_PREV;
	if(ids->songid[0]<1){
		fprintf(stderr,"No results found.\n");
		return PORTAL_RET_PREV;
	}
	ids->tempselectid=insertTempSelect(ids->songid,ids->length);

	x=portal(portalOptions,"Genre");
	cleanTempSelect(ids->tempselectid);
	free(ids->songid);
	return x;
}

void editPortal(){
	struct commandOption portalOptions[]={
		{'s',songPortal,"Edit songs.\nsa\tEdit songs in an album (or albums)\nsr\tEdit songs in an artist (or artists)\nsg\tEdit songs in a genre (or genres)",NULL},
		{'a',albumPortal,"Edit albums",NULL},
		{'r',artistPortal,"Edit artists",NULL},
		{'p',playlistPortal,"Edit playlists",NULL},
		{'g',genrePortal,"Edit genres",NULL},
		{0,NULL,NULL,NULL}
	};
	int ret=1;
	printf("Enter a command. ? for command list.\n");
	if(!arglist[ATYPE].subarg && !(arglist[ATYPE].subarg=malloc(sizeof(char)))){
		debug(2,"Malloc failed (ATYPE subarg).");
		return;
	}
	while(ret){
		ret=portal(portalOptions,"Edit");
		cleanOrphans();
	}
}

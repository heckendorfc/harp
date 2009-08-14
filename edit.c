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

struct IDList {
	int *songid;
	int length;
};

void cleanString(char **ostr){
	char *str=*ostr;
	while(*str!='\n'){str++;}
	*str=0;
	char temp[500];
	db_safe(temp,*ostr,250);
	strcpy(*ostr,temp);
}

int editWarn(char *warn){
	char c[10];
	printf("%s\nDo you wish to continue (y/n)?",warn);
	while(*(fgets(c,sizeof(c),stdin))=='\n'){
		printf("Do you wish to continue (y/n)?");
	}
	if(*c=='y' || *c=='Y')return 1;
	return 0;
}

int getStdArgs(char *args,char *prompt){
	int x;
	for(x=1;x<200 && args[x] && args[x]==' ';x++);
	if(!args[x]){
		printf(prompt);
		fgets(args,200,stdin);
		if(*args=='\n'){
			printf("Aborted\n");
			return -1;
		}
		cleanString(&args);
		return 0;
	}
	else{
		return x;
	}
}

int editSongName(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200];
	int x;

	if((x=getStdArgs(args,"Name: "))<0)return 1;
	args=&args[x];

	if(songids->length>1 && !editWarn("This operation will alter multiple songs.")){
			printf("Aborted\n");
			return 1;
	}

	for(x=0;x<songids->length;x++){
		sprintf(query,"UPDATE Song SET Title='%s' WHERE SongID=%d",args,songids->songid[x]);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return 1;
}

int editSongLocation(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200];
	int x;

	if((x=getStdArgs(args,"Location: "))<0)return 1;
	args=&args[x];

	if(songids->length>1 && !editWarn("This operation will alter multiple songs.")){
			printf("Aborted\n");
			return 1;
	}

	for(x=0;x<songids->length;x++){
		sprintf(query,"UPDATE Song SET Location='%s' WHERE SongID=%d",args,songids->songid[x]);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return 1;
}

int editSongArtist(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200],*ptr;
	int x,albumid,artistid;
	struct dbitem dbi;
	dbiInit(&dbi);

	if((x=getStdArgs(args,"Artist: "))<0)return 1;
	args=&args[x];

	if((artistid=(int)strtol(args,NULL,10))<1)
		artistid=getArtist(args);
	sprintf(query,"SELECT Album.Title FROM Album,Song WHERE Album.AlbumID=Song.AlbumID AND SongID=%d",songids->songid[0]);
	debug3(query);
	if(doQuery(query,&dbi)>0 && fetch_row(&dbi))
		albumid=getAlbum(dbi.row[0],artistid);
	else{
		debug("getAlbum error in editSongArtist");
		dbiClean(&dbi);
		return 1;
	}

	sprintf(query,"UPDATE Song SET AlbumID=%d WHERE SongID=",albumid);
	ptr=&query[41];
	for(x=0;x<songids->length;x++){
		sprintf(ptr,"%d",songids->songid[x]);
		debug3(query);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	dbiClean(&dbi);
	return 1;
}

int editSongAlbum(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200],*ptr;
	int x,albumid,artistid;
	struct dbitem dbi;
	dbiInit(&dbi);

	if((x=getStdArgs(args,"Album title: "))<0)return 1;
	args=&args[x];

	sprintf(query,"SELECT ArtistID from Song NATURAL JOIN AlbumArtist WHERE SongID=%d",songids->songid[0]);
		debug3(query);
	if(doQuery(query,&dbi)<1 || !fetch_row(&dbi) || (artistid=(int)strtol(dbi.row[0],NULL,10))<1){
		debug("getArtist error in getAlbum");
		return 1;
	}

	albumid=getAlbum(args,artistid);
	for(x=0;x<songids->length;x++){
		sprintf(query,"UPDATE Song SET AlbumID=%d WHERE SongID=%d",albumid,songids->songid[x]);
		debug3(query);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	dbiClean(&dbi);
	return 1;
}

int deleteSong(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[100],*ptr;
	int x;
	printf("Delete?: ");
	*args=fgetc(stdin);
	if(*args=='y' || *args=='Y'){
		for(x=0;x<songids->length;x++){
			sprintf(query,"DELETE FROM Song WHERE SongID=%d",songids->songid[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
			sprintf(query,"DELETE FROM PlaylistSong WHERE SongID=%d",songids->songid[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		debug("Songs deleted.");
		return 0;
	}
	return 1;
}

int songActivation(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[100],*ptr;
	int x;
	for(x=0;x<songids->length;x++){
		sprintf(query,"UPDATE Song SET Active=NOT(Active) WHERE SongID=%d",songids->songid[x]);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return 1;
}

int editAlbumArtist(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200],*ptr;
	int x,artistid;
	struct dbitem dbi;
	dbiInit(&dbi);

	if((x=getStdArgs(args,"Artist: "))<0)return 1;
	args=&args[x];

	artistid=getArtist(args);
	for(x=0;x<ids->length;x++){
		sprintf(query,"UPDATE AlbumArtist SET ArtistID=%d WHERE AlbumID=%d",artistid,ids->songid[x]);
		debug3(query);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	dbiClean(&dbi);
	return 1;
}

int editAlbumTitle(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200],*ptr;
	int x,albumid,artistid;
	struct dbitem dbi;
	dbiInit(&dbi);

	if((x=getStdArgs(args,"Album title: "))<0)return 1;
	args=&args[x];

	sprintf(query,"SELECT ArtistID from AlbumArtist WHERE AlbumID=%d",ids->songid[0]);
		debug3(query);
	if(doQuery(query,&dbi)<1 || !fetch_row(&dbi) || (artistid=(int)strtol(dbi.row[0],NULL,10))<1){
		debug("getArtist error in getAlbum");
		return 1;
	}

	albumid=getAlbum(args,artistid);
	for(x=0;x<ids->length;x++){
		sprintf(query,"UPDATE Song SET AlbumID=%d WHERE AlbumID=%d",albumid,ids->songid[x]);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
		sprintf(query,"UPDATE AlbumArtist SET AlbumID=%d WHERE AlbumID=%d",albumid,ids->songid[x]);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
		if(albumid!=ids->songid[x]){
			sprintf(query,"DELETE FROM Album WHERE AlbumID=%d",ids->songid[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
			sprintf(query,"DELETE FROM AlbumArtist WHERE AlbumID=%d",ids->songid[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		ids->songid[x]=albumid;
	}
	ids->length=1;
	dbiClean(&dbi);
	return 1;
}

int editArtistName(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200],*ptr;
	int x,artistid;
	struct dbitem dbi;
	dbiInit(&dbi);

	if((x=getStdArgs(args,"Artist: "))<0)return 1;
	args=&args[x];

	artistid=getArtist(args);
	for(x=0;x<ids->length;x++){
		sprintf(query,"UPDATE AlbumArtist SET ArtistID=%d WHERE ArtistID=%d",artistid,ids->songid[x]);
		debug3(query);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
		if(ids->songid[x]!=artistid){
			sprintf(query,"DELETE FROM Artist WHERE ArtistID=%d",ids->songid[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		ids->songid[x]=artistid;
	}
	ids->length=1;
	dbiClean(&dbi);
	return 1;
}

int editPlaylistName(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200];
	int x;

	if((x=getStdArgs(args,"Name: "))<0)return 1;
	args=&args[x];

	if(ids->length>1 && !editWarn("This operation will alter only the first playlist in the list.")){
			printf("Aborted\n");
			return 1;
	}

	for(x=0;x<ids->length;x++){
		sprintf(query,"UPDATE Playlist SET Title='%s' WHERE PlaylistID=%d",args,ids->songid[x]);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	return 1;
}

int deletePlaylist(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200];
	int x;

	printf("Delete playlist?: ");
	fgets(args,200,stdin);
	if(*args=='y' || *args=='Y'){
		for(x=0;x<ids->length;x++){
			sprintf(query,"DELETE FROM PlaylistSong WHERE PlaylistID=%d",ids->songid[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
			if(ids->songid[x]!=1){ // If Library: delete playlistsongs but leave playlist
				sprintf(query,"DELETE FROM Playlist WHERE PlaylistID=%d",ids->songid[x]);
				sqlite3_exec(conn,query,NULL,NULL,NULL);
			}
		}
		return 0;
	}
	return 1;
}

int editPlaylistSongAdd(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200];
	struct dbitem dbi;
	dbiInit(&dbi);
	int x,order,songid;

	if((x=getStdArgs(args,"Song: "))<0)return 1;
	args=&args[x];

	if(!arglist[ATYPE].subarg)arglist[ATYPE].subarg=alloca(sizeof(char));
	*arglist[ATYPE].subarg='s';
	if((songid=getID(args))<1){
		fprintf(stderr,"No song found.\n");
		return 1;
	}
	*arglist[ATYPE].subarg='p';

	for(x=0;x<ids->length;x++){
		sprintf(query,"SELECT `Order` FROM PlaylistSong WHERE PlaylistID=%d ORDER BY `Order` DESC LIMIT 1",ids->songid[x]);
		doQuery(query,&dbi);
		if(fetch_row(&dbi))
			order=(int)strtol(dbi.row[0],NULL,10)+1;
		else
			order=1;

		sprintf(query,"INSERT INTO PlaylistSong(PlaylistID,SongID,`Order`) VALUES (%d,%d,%d)",ids->songid[x],songid,order);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	dbiClean(&dbi);
	return 1;
}

int editPlaylistSongDelete(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200];
	int x;

	if((x=getStdArgs(args,"Song order: "))<0)return 1;
	args=&args[x];

	if(ids->length>1 && !editWarn("This operation will alter only the first playlist in the list.")){
			printf("Aborted\n");
			return 1;
	}

	x=(int)strtol(args,NULL,10);
	sprintf(query,"DELETE FROM PlaylistSong WHERE `Order`=%d AND PlaylistID=%d",x,ids->songid[0]);
	debug3(query);
	sqlite3_exec(conn,query,NULL,NULL,NULL);
	// Remove the empty place at the old order
	sprintf(query,"UPDATE PlaylistSong SET `Order`=`Order`-1 WHERE `Order`>%d AND PlaylistID=%d",x,ids->songid[0]);
	sqlite3_exec(conn,query,NULL,NULL,NULL);

	return 1;
}

void getPlaylistSongOrders(char *args, int *cur, int*new){
	int x;
	*cur=*new=0;
	for(x=1;args[x] && (args[x]<'0' || args[x]>'9');x++);
	if(!args[x]){
		printf("Current order: ");
		fgets(args,200,stdin);
		if(*args=='\n'){
			printf("Aborted\n");
			*cur=*new=0;
			return;
		}
		cleanString(&args);
		*cur=(int)strtol(args,NULL,10);

		printf("New order: ");
		fgets(args,200,stdin);
		if(*args=='\n'){
			printf("Aborted\n");
			*cur=*new=0;
			return;
		}
		cleanString(&args);
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
			fgets(args,200,stdin);
			if(*args=='\n'){
				printf("Aborted\n");
				*cur=*new=0;
				return;
			}
			cleanString(&args);
			x=0;
		}
		*new=(int)strtol(&args[x],NULL,10);
	}
}

int editPlaylistSongOrder(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	char query[200];
	int x,current_order,new_order;

	if(ids->length>1 && !editWarn("This operation will alter only the first playlist in the list.")){
			printf("Aborted\n");
			return 1;
	}

	getPlaylistSongOrders(args,&current_order,&new_order);
	if(current_order<1 || new_order<1){
		printf("Invalid order\n");
		return 1;
	}

	// Move song out of the way. Order 0 should not be used.
	sprintf(query,"UPDATE PlaylistSong SET `Order`=0 WHERE PlaylistID=%d AND `Order`=%d",ids->songid[0],current_order);
	debug3(query);
	sqlite3_exec(conn,query,NULL,NULL,NULL);
	// Remove the empty place at the old order
	sprintf(query,"UPDATE PlaylistSong SET `Order`=`Order`-1 WHERE `Order`>%d AND PlaylistID=%d",current_order,ids->songid[0]);
	debug3(query);
	sqlite3_exec(conn,query,NULL,NULL,NULL);
	// Make room for the song. TODO: find performance of ignoring the overlap vs testing for overlap in the query.
	sprintf(query,"UPDATE PlaylistSong SET `Order`=`Order`+1 WHERE `Order`>%d AND PlaylistID=%d",new_order-1,ids->songid[0]);
	debug3(query);
	sqlite3_exec(conn,query,NULL,NULL,NULL);
	// Change the song's order
	sprintf(query,"UPDATE PlaylistSong SET `Order`=%d WHERE PlaylistID=%d AND `Order`=0",new_order,ids->songid[0]);
	debug3(query);
	sqlite3_exec(conn,query,NULL,NULL,NULL);

	return 1;
}

void cleanOrphans(){
	// Clean orphaned albums
	sqlite3_exec(conn,"DELETE FROM Album WHERE AlbumID NOT IN (SELECT AlbumID FROM Song)",NULL,NULL,NULL);
	// Clean orphaned albumartists
	sqlite3_exec(conn,"DELETE FROM AlbumArtist WHERE AlbumID NOT IN (SELECT AlbumID FROM Song)",NULL,NULL,NULL);
	// Clean orphaned artists
	sqlite3_exec(conn,"DELETE FROM Artist WHERE ArtistID NOT IN (SELECT ArtistID FROM AlbumArtist)",NULL,NULL,NULL);
}

int *getIDs(char *prompt, int *len){
	int *ids;
	char *song=alloca(sizeof(char)*200);
	printf("%s",prompt);
	while(fgets(song,200,stdin)){
		ids=getMulti(song,len);
		if(*ids>0)
			return ids;
		printf("%s",prompt);
	}
}

int listSongs(char *args, void *data){
	struct IDList *songids=(struct IDList *)data;
	if(!songids || !songids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	struct dbitem dbi;
	dbiInit(&dbi);
	char query[200],*ptr;
	int x=0,limit=songids->length;

	//sprintf(query,"SELECT SongID, SongTitle, Location, AlbumTitle AS Album, ArtistName AS Artist FROM SongPubInfo LIMIT 1");
	sprintf(query,"SELECT SongID, SongTitle, Location, AlbumTitle AS Album, ArtistName AS Artist FROM SongPubInfo WHERE SongID=%d",songids->songid[0]);
	ptr=&query[108];
	if(doQuery(query,&dbi)){
		// Print headers
		printf("[%s] [%s] [%s] [%s] [%s]\n",dbi.row[0],dbi.row[1],dbi.row[2],dbi.row[3],dbi.row[4]);
		// Print first song since we have it already.
		if(fetch_row(&dbi))
			printf("[%s] [%s] [%s] [%s] [%s]\n",dbi.row[0],dbi.row[1],dbi.row[2],dbi.row[3],dbi.row[4]);
	}
	x++;

	for(;x<limit;x++){
		sprintf(ptr,"%d",songids->songid[x]);
		if(doQuery(query,&dbi) && fetch_row(&dbi))
			printf("[%s] [%s] [%s] [%s] [%s]\n",dbi.row[0],dbi.row[1],dbi.row[2],dbi.row[3],dbi.row[4]);
	}
	dbiClean(&dbi);
	return 1;
}

int listAlbums(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	struct dbitem dbi;
	dbiInit(&dbi);
	char query[120],*ptr;
	int x=0;

	sprintf(query,"SELECT AlbumID, Title FROM Album WHERE AlbumID=%d",ids->songid[x]);
	ptr=&query[47];
	debug3(query);
	if(doQuery(query,&dbi)){
		printf("[%s] [%s]\n",dbi.row[0],dbi.row[1]);
		if(fetch_row(&dbi))
			printf("[%s] [%s]\n",dbi.row[0],dbi.row[1]);
	}
	x++;

	for(;x<ids->length;x++){
		sprintf(ptr,"%d",ids->songid[x]);
		if(doQuery(query,&dbi) && fetch_row(&dbi))
			printf("[%s] [%s]\n",dbi.row[0],dbi.row[1]);
	}
	dbiClean(&dbi);
	return 1;
}

int listArtists(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	struct dbitem dbi;
	dbiInit(&dbi);
	char query[120],*ptr;
	int x=0;

	sprintf(query,"SELECT ArtistID, Name FROM Artist WHERE ArtistID=%d",ids->songid[x]);
	ptr=&query[49];
	debug3(query);
	if(doQuery(query,&dbi)){
		printf("[%s] [%s]\n",dbi.row[0],dbi.row[1]);
		if(fetch_row(&dbi))
			printf("[%s] [%s]\n",dbi.row[0],dbi.row[1]);
	}
	x++;

	for(;x<ids->length;x++){
		sprintf(ptr,"%d",ids->songid[x]);
		if(doQuery(query,&dbi) && fetch_row(&dbi))
			printf("[%s] [%s]\n",dbi.row[0],dbi.row[1]);
	}
	dbiClean(&dbi);
	return 1;
}

int listPlaylists(char *args, void *data){
	struct IDList *ids=(struct IDList *)data;
	if(!ids || !ids->songid){
		fprintf(stderr,"no data\n");
		return 1;
	}
	struct dbitem dbi;
	dbiInit(&dbi);
	char query[120],*ptr;
	int x=0;

	// List contents of playlist as well.
	if(args[1]=='C'){
		if(!arglist[ATYPE].subarg)arglist[ATYPE].subarg=alloca(sizeof(char));
		*arglist[ATYPE].subarg='p';
		list(ids->songid,ids->length);
		return 1;
	}

	sprintf(query,"SELECT PlaylistID, Title FROM Playlist WHERE PlaylistID=%d",ids->songid[x]);
	ptr=&query[56];
	debug3(query);
	if(doQuery(query,&dbi)){
		printf("[%s] [%s]\n",dbi.row[0],dbi.row[1]);
		if(fetch_row(&dbi))
			printf("[%s] [%s]\n",dbi.row[0],dbi.row[1]);
	}
	x++;

	for(;x<ids->length;x++){
		sprintf(ptr,"%d",ids->songid[x]);
		debug3(query);
		if(doQuery(query,&dbi) && fetch_row(&dbi))
			printf("[%s] [%s]\n",dbi.row[0],dbi.row[1]);
	}
	dbiClean(&dbi);
	return 1;
}

int songPortal(char *args, void *data){
	/* name location artist album delete active */
	struct IDList *songids=alloca(sizeof(struct IDList));
	struct commandOption portalOptions[]={
		{'L',listSongs,"List affected songs",songids},
		{'t',editSongName,"Change title",songids},
		{'l',editSongLocation,"Change location",songids},
		{'a',editSongAlbum,"Change album",songids},
		{'r',editSongArtist,"Change artist",songids}, 
		{'d',deleteSong,"Delete from database",songids},
		{'v',songActivation,"Toggle activation",songids},
		{0,NULL,NULL,NULL}
	};
	int x,y,*ids,*sids=malloc(sizeof(int)),sidlen,idlen;
	struct dbitem dbi;
	dbiInit(&dbi);

	char query[120],*ptr;

	for(x=1;x<200 && args[x] && args[x]<'0';x++);
	if(!args[x]){
		fprintf(stderr,"Required argument not given.\n");
		return 1;
	}
	if(!arglist[ATYPE].subarg)arglist[ATYPE].subarg=alloca(sizeof(char));
	switch(args[x]){
		case 'a':
			arglist[ATYPE].subarg[0]='a';
			sprintf(query,"SELECT SongID FROM Song INNER JOIN Album ON Album.AlbumID=Song.AlbumID WHERE Album.AlbumID=");
			ptr=&query[91];
			break;
		case 'r':
			arglist[ATYPE].subarg[0]='r';
			sprintf(query,"SELECT SongID FROM Song NATURAL JOIN AlbumArtist WHERE ArtistID=");
			ptr=&query[64];
			break;
		case 's':
			x++;
		default:
			for(;x<200 && args[x] && args[x]==' ';x++);
			arglist[ATYPE].subarg[0]='s';
			sids=getMulti(&args[x],&sidlen);
			if(sids[0]<1)return 1;
			songids->songid=sids;
			songids->length=sidlen;
			x=portal(portalOptions,"song");
			free(sids);
			return x;
	}
	
	// Get SongIDs for album or artist
	for(++x;x<200 && args[x] && args[x]==' ';x++);
	ids=getMulti(&args[x],&idlen);
	if(ids[0]<1){
		fprintf(stderr,"No results found.\n");
		return 1;
	}
	for(sidlen=x=0;x<idlen;x++){
		sprintf(ptr,"%d",ids[x]);
		if(doQuery(query,&dbi)<1)continue;
		debug3(query);
		y=sidlen;
		sidlen+=dbi.row_count;
		sids=realloc(sids,sizeof(int)*(sidlen));
		for(;fetch_row(&dbi);y++)
			sids[y]=(int)strtol(dbi.row[0],NULL,10);
	}
	free(ids);
	if(!sidlen){
		fprintf(stderr,"No results found.\n");
		free(sids);
		return 1;
	}
	songids->songid=sids;
	songids->length=sidlen;
	dbiClean(&dbi);
	x=portal(portalOptions,"song");
	free(sids);
	return x;
}

int albumPortal(char *args, void *data){
	/* artist album */
	struct IDList *ids=alloca(sizeof(struct IDList));
	struct commandOption portalOptions[]={
		{'L',listAlbums,"List affected alubms",ids},
		{'t',editAlbumTitle,"Change title",ids},
		{'r',editAlbumArtist,"Change artist",ids},
		{0,NULL,NULL,NULL}
	};
	int x;

	for(x=1;x<200 && args[x] && args[x]<'0';x++);
	if(!args[x]){
		fprintf(stderr,"Required argument not given.\n");
		return 1;
	}

	if(!arglist[ATYPE].subarg)arglist[ATYPE].subarg=alloca(sizeof(char));
	arglist[ATYPE].subarg[0]='a';

	ids->songid=getMulti(&args[x],&ids->length);
	if(ids->songid[0]<1){
		fprintf(stderr,"No results found.\n");
		return 1;
	}

	return portal(portalOptions,"album");
}

int artistPortal(char *args, void *data){
	/* artist */
	struct IDList *ids=alloca(sizeof(struct IDList));
	struct commandOption portalOptions[]={
		{'L',listArtists,"List affected artists",ids},
		{'n',editArtistName,"Change name",ids},
		{0,NULL,NULL,NULL}
	};
	int x;

	for(x=1;x<200 && args[x] && args[x]<'0';x++);
	if(!args[x]){
		fprintf(stderr,"Required argument not given.\n");
		return 1;
	}

	if(!arglist[ATYPE].subarg)arglist[ATYPE].subarg=alloca(sizeof(char));
	arglist[ATYPE].subarg[0]='r';

	ids->songid=getMulti(&args[x],&ids->length);
	if(ids->songid[0]<1){
		fprintf(stderr,"No results found.\n");
		return 1;
	}

	return portal(portalOptions,"artist");
}

int playlistPortal(char *args, void *data){
	/* name delete song(a,d,o) */
	struct IDList *ids=alloca(sizeof(struct IDList));
	struct commandOption portalOptions[]={
		{'L',listPlaylists,"List affected playlist\nLC\tList the contents of the selected playlist",ids},
		{'n',editPlaylistName,"Change name",ids},
		{'d',deletePlaylist,"Delete playlist from database",ids},
		{'a',editPlaylistSongAdd,"Add a song",ids},
		{'r',editPlaylistSongDelete,"Remove a song from playlist",ids},
		{'o',editPlaylistSongOrder,"Change the order of a song",ids},
		{0,NULL,NULL,NULL}
	};
	int x;

	for(x=1;x<200 && args[x] && args[x]<'0';x++);
	if(!args[x]){
		fprintf(stderr,"Required argument not given.\n");
		return 1;
	}

	if(!arglist[ATYPE].subarg)arglist[ATYPE].subarg=alloca(sizeof(char));
	arglist[ATYPE].subarg[0]='p';

	ids->songid=getMulti(&args[x],&ids->length);
	if(ids->songid[0]<1){
		fprintf(stderr,"No results found.\n");
		return 1;
	}

	return portal(portalOptions,"playlist");
}

void editPortal(){
	struct commandOption portalOptions[]={
		{'s',songPortal,"Edit songs.\nsa\tEdit songs in an album (or albums)\nsr\tEdit songs in an artist (or artists)",NULL},
		{'a',albumPortal,"Edit albums",NULL},
		{'r',artistPortal,"Edit artists",NULL},
		{'p',playlistPortal,"Edit playlists",NULL},
		{0,NULL,NULL,NULL}
	};
	printf("Enter a command. ? for command list.\n");
	while(portal(portalOptions,"edit"));
	cleanOrphans();
}

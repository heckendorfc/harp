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


void cleanString(char **ostr){
	char *str=*ostr;
	while(*str!='\n'){str++;}
	*str=0;
	char temp[500];
	db_safe(temp,*ostr,250);
	strcpy(*ostr,temp);
}

void editSong(int *ids, int len){
	int x,artistid=0,albumid=0;
	char query[400];
	char temp[250];
	char *ptr=temp;
	struct dbitem dbi;
	dbiInit(&dbi);
	
	printf("Editing Song(s). Fields left blank will not be skipped.\n");
	if(len==1){
		printf("Name: ");
		fgets(temp,sizeof(temp),stdin);
		if(*temp!='\n'){
			cleanString(&ptr);
			for(x=0;x<len;x++){
				sprintf(query,"UPDATE Song SET Title='%s' WHERE SongID=%d",temp,ids[x]);
				sqlite3_exec(conn,query,NULL,NULL,NULL);
			}
		}
		printf("Location: ");
		fgets(temp,sizeof(temp),stdin);
		if(*temp!='\n'){
			cleanString(&ptr);
			for(x=0;x<len;x++){
				sprintf(query,"UPDATE Song SET Location='%s' WHERE SongID=%d",temp,ids[x]);
				sqlite3_exec(conn,query,NULL,NULL,NULL);
			}
		}
	}

	printf("Artist: ");
	fgets(temp,sizeof(temp),stdin);
	if(*temp!='\n'){
		cleanString(&ptr);
		artistid=getArtist(temp);
	}
	else{
		sprintf(query,"SELECT ArtistID from Song,AlbumArtist WHERE Song.AlbumID=AlbumArtist.AlbumID AND SongID=%d",ids[0]);
		doQuery(query,&dbi);
		fetch_row(&dbi);
		if((artistid=(int)strtol(dbi.row[0],NULL,0))==0)artistid=1;
	}

	printf("Album title: ");
	fgets(temp,sizeof(temp),stdin);
	if(*temp!='\n'){ // New album name / New album name
		cleanString(&ptr);
		albumid=getAlbum(temp,artistid);
	}
	else{ // Use current album
		sprintf(query,"SELECT AlbumID from Song WHERE SongID=%d",ids[0]);
		doQuery(query,&dbi);
		fetch_row(&dbi);
		albumid=(int)strtol(dbi.row[0],NULL,10);
	}
	for(x=0;x<len;x++){
		sprintf(query,"UPDATE Song SET AlbumID=%d WHERE SongID=%d",albumid,ids[x]);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}

	printf("Delete?[n]: ");
	fgets(temp,sizeof(temp),stdin);
	if(*temp=='y'){
		for(x=0;x<len;x++){
			sprintf(query,"DELETE FROM Song WHERE SongID=%d",ids[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
			sprintf(query,"DELETE FROM PlaylistSong WHERE SongID=%d",ids[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		dbiClean(&dbi);return;
	}

	printf("Toggle Activation?[n]: ");
	fgets(temp,sizeof(temp),stdin);
	if(*temp=='y'){
		for(x=0;x<len;x++){
			sprintf(query,"UPDATE Song SET Active=NOT(Active) WHERE SongID=%d",ids[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
	}
	dbiClean(&dbi);
}

void editAlbum(int *ids, int len){
	int x,tempid,artistid=0;
	char query[400];
	char temp[100];
	char *ptr=temp;
	struct dbitem dbi;
	dbiInit(&dbi);

	printf("Editing Album(s). Fields left blank will not be skipped.\n");
	printf("Artist: ");
	fgets(temp,sizeof(temp),stdin);
	if(*temp!='\n'){
		cleanString(&ptr);
		artistid=getArtist(temp);
	}
	printf("Album title: ");
	fgets(temp,sizeof(temp),stdin);
	if(*temp!='\n' || artistid>0){
		cleanString(&ptr);
		if(artistid<1){ // Use current artist
			sprintf(query,"SELECT ArtistID from AlbumArtist WHERE AlbumID=%d",ids[0]);
			doQuery(query,&dbi);
			fetch_row(&dbi);
			if((artistid=(int)strtol(dbi.row[0],NULL,0))==0)artistid=1;
		}
		tempid=getAlbum(temp,artistid);
		for(x=0;x<len;x++){
			sprintf(query,"UPDATE Song SET AlbumID=%d WHERE AlbumID=%d",tempid,ids[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
			if(tempid!=ids[x]){
				sprintf(query,"DELETE FROM Album WHERE AlbumID=%d",ids[x]);
				sqlite3_exec(conn,query,NULL,NULL,NULL);
				sprintf(query,"DELETE FROM AlbumArtist WHERE AlbumID=%d",ids[x]);
				sqlite3_exec(conn,query,NULL,NULL,NULL);
			}
		}
	}
	dbiClean(&dbi);
}

int findAlbum(char *arg,int id){//doesn't handle duplicate name entries
	char query[401];
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT Album.AlbumID FROM Album,AlbumArtist WHERE Title=\'%s\' AND Album.AlbumID=AlbumArtist.AlbumID AND ArtistID=%d",arg,id);
	doQuery(query,&dbi);
	fetch_row(&dbi);
	if(dbi.row_count==0){
		return 0;
	}
	int newid=(int)strtol(dbi.row[1],NULL,10);
	dbiClean(&dbi);
	return newid;
}

void editArtist(int *ids, int len){
	int x,tempid;
	char query[400];
	char temp[100];
	char *ptr=temp;

	printf("Editing Artist(s). Fields left blank will not be skipped.\n");
	printf("Name: ");
	fgets(temp,sizeof(temp),stdin);
	if(*temp!='\n'){
		cleanString(&ptr);
		tempid=getArtist(temp);
		
		for(x=0;x<len;x++){
			sprintf(query,"UPDATE AlbumArtist SET ArtistID=%d WHERE ArtistID=%d",tempid,ids[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
			if(ids[x]!=tempid){
				sprintf(query,"DELETE FROM Artist WHERE ArtistID=%d",ids[x]);
				sqlite3_exec(conn,query,NULL,NULL,NULL);
			}
		}
	}
}

void editPlaylist(int *ids, int len){
	int x,tempid,ok=0,order=0;
	char query[400];
	char temp[100];
	char *ptr=temp;
	struct dbitem dbi;
	dbiInit(&dbi);
	printf("Editing Playlist(s). Fields left blank will be skipped.\n");

	if(len==1){
		printf("Name: ");
		fgets(temp,sizeof(temp),stdin);
		if(*temp!='\n'){
			cleanString(&ptr);
			for(x=0;x<len;x++){
				sprintf(query,"UPDATE Playlist SET Title='%s' WHERE PlaylistID=%d",temp,ids[x]);
				sqlite3_exec(conn,query,NULL,NULL,NULL);
			}
		}
	}

	printf("Delete?[n]: ");
	fgets(temp,sizeof(temp),stdin);
	if(*temp=='y'){
		for(x=0;x<len;x++){
			sprintf(query,"DELETE FROM PlaylistSong WHERE PlaylistID=%d",ids[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
			if(ids[x]!=1){ // If Library: delete playlistsongs but leave playlist
				sprintf(query,"DELETE FROM Playlist WHERE PlaylistID=%d",ids[x]);
				sqlite3_exec(conn,query,NULL,NULL,NULL);
			}
		}
		dbiClean(&dbi);
		return;
	}

	printf("Song: ");
	while(fgets(temp,sizeof(temp),stdin)){
		if(*temp=='\n')break;
		x=0;
		cleanString(&ptr);//while(temp[x]!='\n')x++;temp[x]=0;
		if((tempid=getID(temp))<1)continue;
		for(x=0;x<len;x++){
			sprintf(query,"SELECT SongID FROM PlaylistSong WHERE PlaylistID=%d AND SongID=%d",ids[x],tempid);
			if(doQuery(query,&dbi)<0)break;
			if(dbi.row_count<1){ // Not in playlist. Add it.
				sprintf(query,"SELECT `Order` FROM PlaylistSong WHERE PlaylistID=%d ORDER BY `Order` DESC LIMIT 1",ids[x]);
				doQuery(query,&dbi);
				if(dbi.row_count>0){ // Get order of new song
					fetch_row(&dbi);
					order=(int)strtol(dbi.row[0],NULL,10);
				}
				else{
					order=1;
				}
				sprintf(query,"INSERT INTO PlaylistSong(PlaylistID,SongID,`Order`) VALUES(%d,%d,%d)",ids[x],tempid,++order);
			}
			else{ // Already in list. Prompt (add(duplicate), remove, order)
				*temp='-';
				printf("Song already in playlist. What would you like to do?\n[a]dd again, [r]emove, change [o]rder: ");
				while(fgets(temp,sizeof(temp),stdin)){
					if(*temp=='\n')break;
					switch(*temp){
						case 'a':
							sprintf(query,"INSERT INTO PlaylistSong(PlaylistID,SongID,`Order`) VALUES(%d,%d,%d)",ids[x],tempid,order);
							ok=1;
							break;
						case 'r':
							sprintf(query,"DELETE FROM PlaylistSong WHERE SongID=%d AND PlaylistID=%d",tempid,ids[x]);
							ok=1;
							break;
						case 'o':
							printf("New order: ");
							fgets(temp,sizeof(temp),stdin);
							cleanString(&ptr);//ok=0;while(temp[ok]!='\n')ok++;ok=temp[ok]=0;
							order=strtol(temp,NULL,10);
							sprintf(query,"UPDATE PlaylistSong SET `Order`=`Order`+1 WHERE `Order`>%d AND PlaylistID=%d",order-1,ids[x]);
							sqlite3_exec(conn,query,NULL,NULL,NULL);
							sprintf(query,"UPDATE PlaylistSong SET `Order`=%d WHERE PlaylistID=%d AND SongID=%d",order,ids[x],tempid);
							ok=1;
							break;
					}
					if(ok)break;
					printf("Invalid option\n[a]dd again, [r]emove, change [o]rder: ");
				}
			}
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		printf("Song (blank to quit): ");
	}
	dbiClean(&dbi);
}

void cleanOrphans(){
	// Clean orphaned albums
	sqlite3_exec(conn,"DELETE FROM Album WHERE AlbumID NOT IN (SELECT AlbumID FROM Song)",NULL,NULL,NULL);
	// Clean orphaned albumartists
	sqlite3_exec(conn,"DELETE FROM AlbumArtist WHERE AlbumID NOT IN (SELECT AlbumID FROM Song)",NULL,NULL,NULL);
	// Clean orphaned artists
	sqlite3_exec(conn,"DELETE FROM Artist WHERE ArtistID NOT IN (SELECT ArtistID FROM AlbumArtist)",NULL,NULL,NULL);
}

int batchEdit(int *ids, int len){
	int x;
	for(x=0;x<len;x++){
		list(ids[x]);
		printf("\n");
	}
	switch(*arglist[ATYPE].subarg){
		case 's':editSong(ids,len);break;
		case 'a':editAlbum(ids,len);break;
		case 'r':editArtist(ids,len);break;
		case 'p':editPlaylist(ids,len);break;
		default:break;
	}
	cleanOrphans();
	return 0;
}

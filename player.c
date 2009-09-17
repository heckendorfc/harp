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


pthread_mutex_t actkey;

typedef int (*function_play)(struct playerHandles *ph, char *key, int *totaltime);
typedef void (*function_exit)(struct playerHandles *ph);
typedef void (*function_seek)(struct playerHandles *ph,int modtime);


int initList(int list, struct dbitem *dbi){
	char query[320];
	dbiInit(dbi);
	if(list){
		sprintf(query,"CREATE TEMP VIEW IF NOT EXISTS TempPlaylistSong AS SELECT PlaylistSongID, SongID, `Order` FROM PlaylistSong WHERE PlaylistID=%d ORDER BY `Order`",list);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
		sprintf(query,"SELECT Song.SongID, Song.TypeID FROM Song, PlaylistSong WHERE Song.SongID=PlaylistSong.SongID AND PlaylistSong.PlaylistID=%d ORDER BY `Order`",list);
		doQuery(query,dbi);
		if(dbi->row_count==0){
			fprintf(stderr,"This does not exist.\n");
			dbiClean(dbi);
			return 1;
		}
	}
	else{
		sprintf(query,"SELECT Song.SongID,Song.TypeID FROM Song, TempPlaylistSong WHERE Song.SongID=TempPlaylistSong.SongID ORDER BY `Order`",list);
		doQuery(query,dbi);
		if(dbi->row_count==0){
			fprintf(stderr,"This does not exist.\n");
			dbiClean(dbi);
			return 1;
		}
	}
	return 0;
}

int player(int list){//list - playlist number
	struct dbitem dbi,updatedbi;
	char query[320];
	char library[255];
	if(initList(list,&dbi))return 0;
	dbiInit(&updatedbi);
	
	// Create playerControl thread
	char key=KEY_NULL;
	struct playercontrolarg pca;
	struct playerHandles ph;
	struct playerflag pflag={0,0,1,32,32};

	ph.ffd=NULL;
	ph.dechandle=NULL;
	ph.pflag=&pflag;
	pca.ph=&ph;
	tcgetattr(0,&pca.orig);
	pca.key=&key;
	pca.listdbi=&dbi;

	pthread_t threads;
	pthread_mutex_init(&actkey,NULL);
	pthread_create(&threads,NULL,(void *)&playerControl,(void*)&pca);
	
	// Play the list!
	int x,fmt,ret,sid,totaltime;

	snd_init(&ph);

	void *module;
	function_play modplay;
	function_exit modexit;

	while(fetch_row(&dbi) && key!=KEY_QUIT){
		// Reset key
		pthread_mutex_lock(&actkey);
			key=KEY_NULL;
		pthread_mutex_unlock(&actkey);
		sid=(int)strtol(dbi.row[0],NULL,10);

		sprintf(query,"SELECT * FROM SongPubInfo WHERE SongID=%d",sid);
		//sprintf(query,"SELECT Song.Title,Song.Location,Album.Title,Artist.Name,FilePlugin.Name,Song.Length,FilePlugin.Library,Song.Rating FROM Song,FilePlugin,Album,Artist,AlbumArtist WHERE Song.AlbumID=Album.AlbumID AND Album.AlbumID=AlbumArtist.AlbumID AND AlbumArtist.ArtistID=Artist.ArtistID AND Song.TypeID=FilePlugin.TypeID AND Song.SongID=%d",sid);
		doQuery(query,&updatedbi);
		if(!fetch_row(&updatedbi))continue;

		printf("\n=====================\n");
		printSongPubInfo(updatedbi.row);
		printf("---------------------\n");

		if((ph.ffd=fopen(updatedbi.row[1],"rb"))==NULL){
			fprintf(stderr,"Failed to open file\n");
			continue;
		}

		totaltime=(int)strtol(updatedbi.row[5],NULL,10);
		ph.pflag->rating=(int)strtol(updatedbi.row[7],NULL,10);

		updatedbi.current_row=updatedbi.column_count; // Reset for getPlugin
		while((x=getPlugin(&updatedbi,6,&module))){
			if(x>1)continue;
			modplay=dlsym(module,"plugin_run");
			if(!modplay){
				debug("Plugin does not contain plugin_run().\n");
				//debug(dlerror());
				ret=-1;
			}
			else{
				pca.decoder=module;
				ret=modplay(&ph,&key,&totaltime);
				pca.decoder=NULL;
				dlclose(module);
				break;
			}
			dlclose(module);
		}
		if(!x)
			ret=-1;

		printf("\n");
		modplay=NULL;
		modexit=NULL;
		fclose(ph.ffd);
		ph.ffd=NULL;
		ph.dechandle=NULL;

		if(ret!=DEC_RET_ERROR){ // Update stats
			switch(ret){
					// Normal play.
				case DEC_RET_SUCCESS:sprintf(query,"UPDATE Song SET PlayCount=PlayCount+1, LastPlay=%d, Length=%d WHERE SongID=%d",(int)time(NULL),totaltime,sid);break;
					// Next key
				case DEC_RET_NEXT:sprintf(query,"UPDATE Song SET SkipCount=SkipCount+1, LastPlay=%d WHERE SongID=%d",(int)time(NULL),sid);break;
					// Next key without update[playcount]
				case DEC_RET_NEXT_NOUP:
				default:sprintf(query,"UPDATE Song SET LastPlay=%d WHERE SongID=%d",(int)time(NULL),sid);break;
			}
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}	
		sprintf(query,"UPDATE Song SET Rating=%d WHERE SongID=%d",ph.pflag->rating,sid);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	snd_close(&ph);

	pthread_cancel(threads);
	dbiClean(&dbi);
	dbiClean(&updatedbi);
	// Reset term settings
		// In case user used Q
	tcsetattr(0,TCSANOW,&pca.orig);
	pthread_mutex_destroy(&actkey);
	printf("Exiting.\n");
	return 0;
}

void playerControl(void *arg){
	char temp;
	struct playercontrolarg *pca=(struct playercontrolarg*)arg;
	
	// Set new term settings
	struct termios orig,new;
	new=pca->orig;
	new.c_lflag&=(~ICANON);
	new.c_lflag&=(~ECHO);
	new.c_cc[VTIME]=0;
	new.c_cc[VMIN]=1;
	tcsetattr(0,TCSANOW,&new);

	while(1){
		temp=fgetc(stdin);
		if(getSystemKey(temp,pca))
			continue;
		pthread_mutex_lock(&actkey);
			*pca->key=temp;
		pthread_mutex_unlock(&actkey);
		if(temp==KEY_QUIT){
			pca->ph->pflag->pause=0;
			debug3("playerControl found quit");
			break;
		}
	}
	debug3("playerControl quitting");
	
	// Reset term settings
	tcsetattr(0,TCSANOW,&orig);

	pthread_exit((void *) 0);
}

void advseek(char *com, struct playercontrolarg *pca){
	int x,y;
	for(y=1;y<50 && com[y] && (com[y]<'0' || com[y]>'9');y++);
	int time=(int)strtol(&com[y],NULL,10);
	if(time<=0)return;
	
	if(com[0]==KEY_SEEK_DN)
		time*=-1;

	if(!pca->decoder)return;
	function_seek seek;
	seek=dlsym(pca->decoder,"plugin_seek");
	if(seek)seek(pca->ph,time);
}

void jump(char *com, struct playercontrolarg *pca){
	struct dbitem dbi;
	char query[200];
	dbiInit(&dbi);

	int x,y;
	for(y=1;y<50 && com[y] && (com[y]<'0' || com[y]>'9');y++);
	int dest=(int)strtol(&com[y],NULL,10);
	if(dest<=0)return;

	if(com[1]=='o'){ // Jump by Order. Find the SongID first (listdbi only uses SongID and TypeID).
		sprintf(query,"SELECT SongID FROM TempPlaylistSong WHERE `Order`=%d",dest);
		debug3(query);
		if(doQuery(query,&dbi)<1){
			dbiClean(&dbi);
			return;
		}
		dest=(int)strtol(dbi.result[1],NULL,10);
		dbiClean(&dbi);
		if(dest<=0)return;
	}

	int interval=pca->listdbi->column_count;
	for(x=interval;x<(pca->listdbi->row_count+1)*interval;x+=interval){
		if((int)strtol(pca->listdbi->result[x],NULL,10)==dest){
			pca->listdbi->current_row=x;
			pthread_mutex_lock(&actkey);
				*pca->key=(*com=='j')?KEY_NEXT:KEY_NEXT_NOUP;
			pthread_mutex_unlock(&actkey);
			break;
		}
	}
}

void listtemp(char *com, struct playercontrolarg *pca){
	struct dbitem dbi;
	char query[200];
	dbiInit(&dbi);

	int x,y,limit,*exception=alloca(sizeof(int)*10);
	for(y=2;y<10;y++)exception[y]=listconf.exception;
	exception[0]=exception[1]=1;

	for(y=1;y<50 && com[y] && (com[y]<'0' || com[y]>'9');y++);
	limit=(int)strtol(&com[y],NULL,10);
	if(limit<=0)limit=30;
	switch(com[1]){
		case 'h': // Head
			sprintf(query,"SELECT `Order`,SongID,SongTitle,AlbumTitle,ArtistName FROM TempPlaylistSong NATURAL JOIN SongPubInfo ORDER BY `Order` LIMIT %d",limit);
			break;
		case 't': // Tail
			x=pca->listdbi->row_count-limit;
			if(x<0)x=0;
			sprintf(query,"SELECT `Order`,SongID,SongTitle,AlbumTitle,ArtistName FROM TempPlaylistSong NATURAL JOIN SongPubInfo ORDER BY `Order` LIMIT %d,%d",x,limit);
			break;
		case 'r': // Relative
		default:
			x=(pca->listdbi->current_row-(pca->listdbi->column_count*2))/pca->listdbi->column_count;
			sprintf(query,"SELECT `Order`,SongID,SongTitle,AlbumTitle,ArtistName FROM TempPlaylistSong NATURAL JOIN SongPubInfo ORDER BY `Order` LIMIT %d,%d",x,limit);
			break;
	}
	debug3(query);
	doTitleQuery(query,&dbi,exception,listconf.maxwidth);
}

void getCommand(struct playercontrolarg *pca){
	struct termios old;
	tcsetattr(0,TCSANOW,&pca->orig);

	char *ptr,com[50];
	ptr=com;
	int oldupdate=pca->ph->pflag->update;
	pca->ph->pflag->update=0;
	printf(":");
	fflush(stdout);
	if(fgets(com,50,stdin)){
		int x,y;

		for(x=0;x<50 && com[x]!='\n' && com[x];x++);
		if(x<50)com[x]=0;
		if(*com=='l'){ // List the playlist
			listtemp(ptr,pca);
		}
		else if(*com=='j' || *com=='J'){ // Jump to a different song
			jump(ptr,pca);
		}
		else if(*com==KEY_SEEK_DN || *com==KEY_SEEK_UP){ // Seek with specified value
			advseek(ptr,pca);
		}
	}
	tcgetattr(0,&old);
	old.c_lflag&=(~ICANON);
	old.c_lflag&=(~ECHO);
	old.c_cc[VTIME]=0;
	old.c_cc[VMIN]=1;
	tcsetattr(0,TCSANOW,&old);
	pca->ph->pflag->update=oldupdate;
}

int getSystemKey(char key, struct playercontrolarg *pca){
	function_seek seek;
	switch(key){
		case KEY_VOLUP:
			debug3("KEY_VOLUP");
			changeVolume(5);
			return 1;
		case KEY_VOLDN:
			debug3("KEY_VOLDN");
			changeVolume(-5);
			return 1;
		case KEY_MUTE:
			debug3("KEY_MUTE");
			toggleMute(&pca->ph->pflag->mute);
			return 1;
		case KEY_PAUSE: 
			pca->ph->pflag->pause=!pca->ph->pflag->pause;
			return 1;
		case KEY_PREV:
			if(!pca->decoder)return 1;
			seek=dlsym(pca->decoder,"plugin_seek");
			if(seek){
				pca->ph->pflag->pause=0;
				seek(pca->ph,0);
			}
			return 1;
		case KEY_SEEK_UP:
			if(!pca->decoder)return 1;
			seek=dlsym(pca->decoder,"plugin_seek");
			if(seek){
				pca->ph->pflag->pause=0;
				seek(pca->ph,20);
			}
			return 1;
		case KEY_SEEK_DN:
			if(!pca->decoder)return 1;
			seek=dlsym(pca->decoder,"plugin_seek");
			if(seek){
				pca->ph->pflag->pause=0;
				seek(pca->ph,-20);
			}
			return 1;
		case KEY_RATEUP:
			pca->ph->pflag->rating=pca->ph->pflag->rating==10?10:pca->ph->pflag->rating+1;
			fprintf(stdout,"\r                               Rating: %d/10  ",pca->ph->pflag->rating);
			fflush(stdout);
			return 1;
		case KEY_RATEDN:
			pca->ph->pflag->rating=pca->ph->pflag->rating==0?0:pca->ph->pflag->rating-1;
			fprintf(stdout,"\r                               Rating: %d/10  ",pca->ph->pflag->rating);
			fflush(stdout);
			return 1;
		case KEY_COMMAND:
			getCommand(pca);
			return 1;
		default:return 0;
	}	
}

struct randgroup{
	char *str;
	int ran;
};

int rg_sort(const void *one, const void *two){
	struct randgroup *a=(struct randgroup *)one;
	struct randgroup *b=(struct randgroup *)two;
	return a->ran < b->ran? -1: a->ran > b->ran? 1: 0;
}

void randomize(struct randgroup *rg, char **col, const int items){
	int x;
	srandom((unsigned int)time(NULL));
	for(x=0;x<items;x++){
		rg[x].str=col[x];
		rg[x].ran=(int)random();
	}
	
	qsort(rg,items,sizeof(struct randgroup),rg_sort);
}

void shuffle(int list){
	struct dbitem dbi;
	char query[250];
	char **col;
	int x,total_songs;
	dbiInit(&dbi);

	if(list)
		sprintf(query,"SELECT SongID FROM PlaylistSong WHERE PlaylistID=%d",list);
	else
		sprintf(query,"SELECT SongID FROM TempPlaylistSong");

	debug3(query);
	doQuery(query,&dbi);
	total_songs=dbi.row_count;
	
	col=fetch_column_at(&dbi,0);
	struct randgroup *rg=alloca(sizeof(struct randgroup)*total_songs);
	randomize(rg,col,total_songs);
	
	if(list)
		createTempPlaylistSong();
	else
		sqlite3_exec(conn,"DELETE FROM TempPlaylistSong",NULL,NULL,NULL); 

	/*for(x=0;x<total_songs;x++){
		sprintf(query,"INSERT INTO PlaylistSong(SongID,PlaylistID,`Order`) VALUES(%d,%d,%d)",(int)strtol(rg[x].str,NULL,10),0,x);
		debug3(query);
		doQuery(query,&insertdbi);
	}*/

	int currentlimit=x=0;
	while(x<total_songs){
		if((currentlimit+=100)>total_songs)currentlimit=total_songs;
		sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
		for(;x<currentlimit;x++){
			sprintf(query,"INSERT INTO TempPlaylistSong(SongID,`Order`) VALUES(%d,%d)",(int)strtol(rg[x].str,NULL,10),x+1);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
	}
		
	free(col);
	dbiClean(&dbi);
}

struct zrandgroup{
	int id;
	int playcount;
	int skipcount;
	int rating;
	int lastplay;
	int ran;
};

int zrg_random_sort(const void *one, const void *two){
	struct zrandgroup *a=(struct zrandgroup *)one;
	struct zrandgroup *b=(struct zrandgroup *)two;
	return a->ran < b->ran? 1: a->ran > b->ran? -1: 0;
}

int zrg_skip_count_sort(const void *one, const void *two){
	struct zrandgroup *a=(struct zrandgroup *)one;
	struct zrandgroup *b=(struct zrandgroup *)two;
	return a->skipcount < b->skipcount? 1: a->skipcount > b->skipcount? -1: zrg_random_sort(one,two);
}

int zrg_rating_sort(const void *one, const void *two){
	struct zrandgroup *a=(struct zrandgroup *)one;
	struct zrandgroup *b=(struct zrandgroup *)two;
	return a->rating < b->rating? 1: a->rating > b->rating? -1: zrg_random_sort(one,two);
}

int zrg_play_count_sort(const void *one, const void *two){
	struct zrandgroup *a=(struct zrandgroup *)one;
	struct zrandgroup *b=(struct zrandgroup *)two;
	return a->playcount < b->playcount? 1: a->playcount > b->playcount? -1: zrg_random_sort(one,two);
}

int zrg_lastplay_sort(const void *one, const void *two){
	struct zrandgroup *a=(struct zrandgroup *)one;
	struct zrandgroup *b=(struct zrandgroup *)two;
	return a->lastplay < b->lastplay? -1: a->lastplay > b->lastplay? 1: zrg_random_sort(one,two);
}

void zrandomize(struct zrandgroup *rg, const int items, int mod){
	int x,test;
	const int lastplaymod=mod*.25; // Random 25% bonus
	const int skipcountmod=mod*.15; // Random 15% deduction
	const int ratingmod=mod*.02; // Random 2% (multipled by rating number) bonus
	const int playcountmod=mod*.05; // Random 5% bonus

	char msg[200];

	srandom((unsigned int)time(NULL));
	for(x=0;x<items;x++){ // Initial random values (order shouldn't matter)
		rg[x].ran+=(int)random()%mod;
	}

	// LastPlay Bonus (already in order)
	for(x=(items-1)/5;x<items;x++)if(rg[x].lastplay!=0)break; // Min 20%; Max total unplayed
	sprintf(msg,"lastplay\nx: %d | 20%: %d | most: %d | least: %d | tail: %d",x,(items-1)/5,rg[0].lastplay,rg[x].lastplay,rg[items-1].lastplay);
	debug(msg);
	for(;x>=0;x--){
	//for(x=items/5;x>0;x--){
		rg[x].ran+=(int)random()%lastplaymod;
	}
	sprintf(msg,"x: %d | mod: %d\n",x,lastplaymod);
	debug3(msg);

	// Rating Bonus
	qsort(rg,items,sizeof(struct zrandgroup),zrg_rating_sort);
	for(x=items-1;x>=0;x--)if(rg[x].rating!=0)break; // Ignore songs with 0 rating
	sprintf(msg,"rating\nx: %d | most: %d | least: %d | tail: %d",x,rg[0].rating,rg[x].rating,rg[items-1].rating);
	debug(msg);
	for(;x>=0;x--){
		rg[x].ran+=(int)random()%(ratingmod*rg[x].rating);
	}
	sprintf(msg,"x: %d | mod: %d * rating\n",x,ratingmod);
	debug(msg);

	//PlayCount Bonus
	qsort(rg,items,sizeof(struct zrandgroup),zrg_play_count_sort);
	x=(items-1)/5;
	test=rg[0].playcount;
	if(rg[x].playcount==test && test!=0){
		for(;x<items;x++){
			if(rg[x].playcount!=test){
				x--;break;
			} // If top 20% are the same, add the rest.
		}
	}
	else{
		for(;x>=0;x--){
			if(rg[x].playcount!=0)break; // Max 20%; Ignore unplayed
		}
	}
	sprintf(msg,"playcount\nx: %d | 20%: %d | most: %d | least: %d | tail: %d",x,(items-1)/5,rg[0].playcount,rg[x].playcount,rg[items-1].playcount);
	debug(msg);
	for(;x>=0;x--){
		rg[x].ran+=(int)random()%playcountmod;
	}
	sprintf(msg,"x: %d | mod: %d\n",x,playcountmod);
	debug(msg);

	//SkipCount Penalty
	qsort(rg,items,sizeof(struct zrandgroup),zrg_skip_count_sort);
	x=(items-1)/5;
	test=rg[0].skipcount;
	if(rg[x].skipcount==test && test!=0){
		for(;x>=0;x++){
			if(rg[x].skipcount!=test){  // If top 20% are the same, add the rest.
				x--;
				break;
			}
		}
	}
	else{
		for(;x>=0;x--){
			if(rg[x].skipcount!=0)break; // Max 20%; Ignore unskipped
		}
	}
	sprintf(msg,"skipcount\nx: %d | 20%: %d | most: %d | least: %d | tail: %d",x,(items-1)/5,rg[0].skipcount,rg[x].skipcount,rg[items-1].skipcount);
	debug(msg);
	for(;x>=0;x--){
		rg[x].ran-=(int)random()%skipcountmod;
	}
	sprintf(msg,"x: %d | mod: -%d\n",x,skipcountmod);
	debug(msg);
	
	// Put in random order
	qsort(rg,items,sizeof(struct zrandgroup),zrg_random_sort);
}

void zshuffle(int list){
	struct dbitem dbi;
	char query[250];
	int x,total_songs;
	dbiInit(&dbi);

	// Set up random list
	if(list)
		sprintf(query,"SELECT Song.SongID,LastPlay,SkipCount,Rating,PlayCount FROM PlaylistSong,Song WHERE PlaylistID=%d AND PlaylistSong.SongID=Song.SongID ORDER BY LastPlay ASC",list);
	else
		sprintf(query,"SELECT Song.SongID,LastPlay,SkipCount,Rating,PlayCount FROM TempPlaylistSong,Song WHERE TempPlaylistSong.SongID=Song.SongID ORDER BY LastPlay ASC",list);

	doQuery(query,&dbi);
	total_songs=dbi.row_count;
	struct zrandgroup *rg=calloc(total_songs,sizeof(struct zrandgroup));
	for(x=0;fetch_row(&dbi);x++){
		rg[x].id=(int) strtol(dbi.row[0],NULL,10);
		rg[x].playcount=(int)strtol(dbi.row[4],NULL,10);
		rg[x].skipcount=(int)strtol(dbi.row[2],NULL,10);
		rg[x].rating=(int)strtol(dbi.row[3],NULL,10);
		rg[x].lastplay=(int)strtol(dbi.row[1],NULL,10);
	}
	dbiClean(&dbi);
	dbiInit(&dbi);
	zrandomize(rg,total_songs,1<<(sizeof(int)*6));

	if(list)
		createTempPlaylistSong();
	else
		sqlite3_exec(conn,"DELETE FROM TempPlaylistSong",NULL,NULL,NULL); 

	int currentlimit=x=0;
	while(x<total_songs){
		if((currentlimit+=100)>total_songs)currentlimit=total_songs;
		sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
		for(;x<currentlimit;x++){
			sprintf(query,"INSERT INTO TempPlaylistSong(SongID,`Order`) VALUES(%d,%d)",rg[x].id,x+1);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
	}

	free(rg);
	dbiClean(&dbi);
}

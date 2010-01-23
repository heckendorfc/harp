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

#include "player.h"
#include "defs.h"
#include "dbact.h"
#include "util.h"
#include "admin.h"
#include "sndutil.h"

pthread_mutex_t actkey;

struct play_song_args{
	struct playerHandles *ph;
	struct playercontrolarg *pca;
};

static int initList(int list, char *query){
	// If we shuffled, TempPlaylistSong will be a temp table. list will be 0.
	if(list){
		sprintf(query,"CREATE TEMP VIEW IF NOT EXISTS TempPlaylistSong AS SELECT PlaylistSongID, SongID, `Order` FROM PlaylistSong WHERE PlaylistID=%d ORDER BY `Order`",list);
		sqlite3_exec(conn,query,NULL,NULL,NULL);
		sprintf(query,"SELECT Song.SongID, `Order` FROM Song, PlaylistSong WHERE Song.SongID=PlaylistSong.SongID AND PlaylistSong.PlaylistID=%d ORDER BY `Order`",list);
	}
	else{
		sprintf(query,"SELECT Song.SongID,`Order` FROM Song, TempPlaylistSong WHERE Song.SongID=TempPlaylistSong.SongID ORDER BY `Order`");
	}
	return 0;
}


int play_song(void *data, int col_count, char **row, char **titles){
	struct play_song_args *psargs=(struct play_song_args *)data;
	int x,ret,sid,totaltime,filetype;
	char query[200];
	struct dbitem dbi;
	struct pluginitem *pi_ptr;

	if(*psargs->pca->key==KEY_QUIT)return 1;

	// Reset key
	pthread_mutex_lock(&actkey);
		*psargs->pca->key=KEY_NULL;
	pthread_mutex_unlock(&actkey);
	psargs->pca->next_order=psargs->pca->cur_order=strtol(row[1],NULL,10);

	dbiInit(&dbi);
	sid=(int)strtol(row[0],NULL,10);

	// Row information done. From now on we use the dbi
	sprintf(query,"SELECT * FROM SongPubInfo WHERE SongID=%d LIMIT 1",sid);
	doQuery(query,&dbi);
	if(!fetch_row(&dbi))
		return 0;

	printf("\n=====================\n");
	printSongPubInfo(dbi.row);
	printf("---------------------\n");

	totaltime=(int)strtol(dbi.row[5],NULL,10);
	psargs->ph->pflag->rating=(int)strtol(dbi.row[7],NULL,10);

	//filetype=strtol(dbi.row[6],NULL,10);
	filetype=getFileTypeByName(dbi.row[4]);
	findPluginIDByType(0); // Reset


	while((pi_ptr=findPluginByID(psargs->ph->plugin_head,findPluginIDByType(filetype)))){
		if((psargs->ph->ffd=pi_ptr->modopen(dbi.row[1],"rb"))!=NULL){
			psargs->pca->decoder=pi_ptr;
			ret=pi_ptr->modplay(psargs->ph,psargs->pca->key,&totaltime);
			pi_ptr->modclose(psargs->ph->ffd);
			psargs->pca->decoder=NULL;
			break;
		}
		else{
			fprintf(stderr,"Failed to open file\n");
			dbiClean(&dbi);
			return 0;
		}
	}
	if(!x)
		ret=-1;

	printf("\n");
	dbiClean(&dbi);
	psargs->ph->ffd=NULL;
	psargs->ph->dechandle=NULL;
	psargs->ph->pflag->exit=DEC_RET_SUCCESS;

	if(ret!=DEC_RET_ERROR){ // Update stats
		switch(ret){
			case DEC_RET_SUCCESS: // Normal play.
				if(totaltime>0)
					sprintf(query,"UPDATE Song SET PlayCount=PlayCount+1, LastPlay=%d, Length=%d WHERE SongID=%d",(int)time(NULL),totaltime,sid);
				else
					sprintf(query,"UPDATE Song SET PlayCount=PlayCount+1, LastPlay=%d, WHERE SongID=%d",(int)time(NULL),sid);
				break;
			case DEC_RET_NEXT: // Next key
				sprintf(query,"UPDATE Song SET SkipCount=SkipCount+1, LastPlay=%d WHERE SongID=%d",(int)time(NULL),sid);
				break;
			case DEC_RET_NEXT_NOUP: // Next key without update[playcount]
			default:
				sprintf(query,"UPDATE Song SET LastPlay=%d WHERE SongID=%d",(int)time(NULL),sid);
				break;
		}
		sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	sprintf(query,"UPDATE Song SET Rating=%d WHERE SongID=%d",psargs->ph->pflag->rating,sid);
	sqlite3_exec(conn,query,NULL,NULL,NULL);
	
	if(ret==DEC_RET_ERROR || psargs->pca->next_order!=psargs->pca->cur_order)
		return 1;
	else
		return 0;
}

int player(int list){//list - playlist number
	char *query; // Why aren't we using []?
	char library[255];
	
	// Create playerControl thread
	char key=KEY_NULL;
	struct play_song_args psargs;
	struct playercontrolarg pca;
	struct playerHandles ph;
	struct playerflag pflag={0,0,1,0,DEC_RET_SUCCESS,32,32};

	if(!(query=malloc(sizeof(char)*320))){
		debug(2,"Malloc failed (player query).");
		return 1;
	}
	else if(initList(list,query) ||
			(!(ph.plugin_head=openPlugins())))
		return 0;

	ph.ffd=NULL;
	ph.device=arglist[ADEVICE].subarg;
	ph.dechandle=NULL;
	ph.pflag=&pflag;
	pca.ph=&ph;
	tcgetattr(0,&pca.orig);
	pca.key=&key;
	psargs.ph=&ph;
	psargs.pca=&pca;
	

	pthread_t threads;
	pthread_mutex_init(&actkey,NULL);
	pthread_create(&threads,NULL,(void *)&playerControl,(void*)&pca);
	
	// Play the list!
	if(snd_init(&ph)){
		fprintf(stderr,"snd_init failed");
		closePluginList(ph.plugin_head);
		free(query);
		return 1;
	}

	// Run the song playing query. Loop for jumping.
	while(sqlite3_exec(conn,query,play_song,&psargs,NULL)==SQLITE_ABORT &&
			pca.next_order!=pca.cur_order){
		sprintf(query,"SELECT Song.SongID,`Order` FROM Song NATURAL JOIN TempPlaylistSong WHERE `Order`>=%d ORDER BY `Order`",pca.next_order);
	}

	closePluginList(ph.plugin_head);
	snd_close(&ph);
	pthread_cancel(threads);
	// Reset term settings
	tcsetattr(0,TCSANOW,&pca.orig);
	pthread_mutex_destroy(&actkey);
	printf("Exiting.\n");
	free(query);
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
			debug(2,"playerControl found quit");
			break;
		}
	}
	debug(2,"playerControl quitting");
	
	// Reset term settings
	tcsetattr(0,TCSANOW,&orig);

	pthread_exit((void *) 0);
}

static void writelist(char *com, struct playercontrolarg *pca){
	int x,y,limit;
	unsigned int order;
	FILE *ffd;
	struct dbitem dbi;
	char query[200],filename[30];
	dbiInit(&dbi);

	for(y=1;y<50 && com[y] && (com[y]<'0' || com[y]>'9');y++);
	limit=(int)strtol(&com[y],NULL,10);
	if(limit<=0)limit=50;
	//for(x=1;x<y && com[x];y++);
	switch(com[1]){
		case 'h':
			sprintf(query,"SELECT `Order` AS \"#\",SongID,Title,Location,Rating,PlayCount,SkipCount,LastPlay FROM Song NATURAL JOIN TempPlaylistSong ORDER BY `Order` LIMIT %d",limit);
			break;
		case 't':
			sqlite3_exec(conn,query,uint_return_cb,&order,NULL);
			if(x<0)x=0;
			sprintf(query,"SELECT `Order` AS \"#\",SongID,Title,Location,Rating,PlayCount,SkipCount,LastPlay FROM Song NATURAL JOIN TempPlaylistSong ORDER BY `Order` LIMIT %d",limit);
			break;
		case 'r':
		default:
			sqlite3_exec(conn,query,uint_return_cb,&order,NULL);
			sprintf(query,"SELECT `Order` AS \"#\",SongID,Title,Location,Rating,PlayCount,SkipCount,LastPlay FROM Song NATURAL JOIN TempPlaylistSong ORDER BY `Order` LIMIT %d",limit);
			break;
	}
	sprintf(filename,"harp_stats_%d.csv",(int)time(NULL));
	if((ffd=fopen(filename,"w"))==NULL){
		fprintf(stderr,"Failed to open file\n");
		return;
	}
	fputs("ORDER\tID\tTITLE\tLOCATION\tRATING\tPLAYCOUNT\tSKIPCOUNT\tLASTPLAY\n",ffd);
	sqlite3_exec(conn,query,write_stats_cb,ffd,NULL);
	fclose(ffd);
}

static void advseek(char *com, struct playercontrolarg *pca){
	int x,y;
	for(y=1;y<50 && com[y] && (com[y]<'0' || com[y]>'9');y++);
	int time=(int)strtol(&com[y],NULL,10);
	if(time<=0)return;
	
	if(com[0]==KEY_SEEK_DN)
		time*=-1;

	if(!pca->decoder)return;
	function_seek seek;
	seek=dlsym(pca->decoder,"plugin_seek");
	if(seek){
		pca->ph->pflag->pause=0;
		seek(pca->ph,time);
	}
}

static void jump(char *com, struct playercontrolarg *pca){
	char query[200];

	int x,y;
	for(y=1;y<50 && com[y] && (com[y]<'0' || com[y]>'9');y++);
	int dest=(int)strtol(&com[y],NULL,10);
	if(dest<=0)return;

	if(com[1]=='s'){ // Jump by SongID. Find the Order first,
		sprintf(query,"SELECT `Order` FROM TempPlaylistSong WHERE SongID=%d",dest);
		debug(3,query);
		sqlite3_exec(conn,query,uint_return_cb,&dest,NULL);
		if(dest<=0)return;
	}

	pca->next_order=dest;

	pthread_mutex_lock(&actkey);
		*pca->key=(*com=='j')?KEY_NEXT:KEY_NEXT_NOUP;
	pthread_mutex_unlock(&actkey);
	pca->ph->pflag->exit=*pca->key;
	pca->ph->pflag->pause=0;
}

static void listtemp(char *com, struct playercontrolarg *pca){
	char query[200];
	int x,y,limit,exception[10];
	unsigned int order;
	for(y=2;y<10;y++)exception[y]=listconf.exception;
	exception[0]=exception[1]=1;

	for(y=1;y<50 && com[y] && (com[y]<'0' || com[y]>'9');y++);
	limit=(int)strtol(&com[y],NULL,10);
	if(limit<=0)limit=30;
	switch(com[1]){
		case 'h': // Head
			sprintf(query,"SELECT `Order` AS \"#\",SongID,SongTitle,AlbumTitle,ArtistName FROM TempPlaylistSong NATURAL JOIN SongPubInfo ORDER BY `Order` LIMIT %d",limit);
			break;
		case 't': // Tail
			sqlite3_exec(conn,"SELECT MAX(`Order`) FROM TempPlaylistSong",uint_return_cb,&order,NULL);
			sprintf(query,"SELECT `Order` AS \"#\",SongID,SongTitle,AlbumTitle,ArtistName FROM TempPlaylistSong NATURAL JOIN SongPubInfo WHERE `Order`>%d ORDER BY `Order` LIMIT %d",order-limit,limit);
			break;
		case 'r': // Relative
		default:
			sprintf(query,"SELECT `Order` AS \"#\",SongID,SongTitle,AlbumTitle,ArtistName FROM TempPlaylistSong NATURAL JOIN SongPubInfo WHERE `Order`>=%d ORDER BY `Order` LIMIT %d",pca->cur_order,limit);
			break;
	}
	debug(3,query);
	doTitleQuery(query,exception,listconf.maxwidth);
}

static void getCommand(struct playercontrolarg *pca){
	struct termios old;
	tcsetattr(0,TCSANOW,&pca->orig);

	char *ptr,com[50];
	bzero(com,50);
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
		else if(*com=='w'){ // Write out information about the playlist
			writelist(ptr,pca);
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
	int oldrating;
	switch(key){
		case KEY_VOLUP:
			changeVolume(pca->ph,5);
			break;
		case KEY_VOLDN:
			changeVolume(pca->ph,-5);
			break;
		case KEY_MUTE:
			toggleMute(pca->ph,&pca->ph->pflag->mute);
			break;
		case KEY_PAUSE: 
			pca->ph->pflag->pause=!pca->ph->pflag->pause;
			break;
		case KEY_PREV:
			if(!pca->decoder)break;
			pca->ph->pflag->pause=0;
			pca->decoder->modseek(pca->ph,0);
			break;
		case KEY_SEEK_UP:
			if(!pca->decoder)break;
			pca->ph->pflag->pause=0;
			pca->decoder->modseek(pca->ph,20);
			break;
		case KEY_SEEK_DN:
			if(!pca->decoder)break;
			pca->ph->pflag->pause=0;
			pca->decoder->modseek(pca->ph,-20);
			break;
		case KEY_RATEUP:
			oldrating=pca->ph->pflag->rating;
			pca->ph->pflag->rating=pca->ph->pflag->rating==10?10:pca->ph->pflag->rating+1;
			fprintf(stdout,"\r                               Rating: %d/10 -> %d/10  ",oldrating,pca->ph->pflag->rating);
			fflush(stdout);
			break;
		case KEY_RATEDN:
			oldrating=pca->ph->pflag->rating;
			pca->ph->pflag->rating=pca->ph->pflag->rating==0?0:pca->ph->pflag->rating-1;
			fprintf(stdout,"\r                               Rating: %d/10 -> %d/10  ",oldrating,pca->ph->pflag->rating);
			fflush(stdout);
			break;
		case KEY_COMMAND:
			getCommand(pca);
			break;
		case KEY_QUIT:
			pca->ph->pflag->exit=DEC_RET_ERROR;
			pca->ph->pflag->pause=0;
			return 0;
		case KEY_NEXT:
			pca->ph->pflag->exit=DEC_RET_NEXT;
			pca->ph->pflag->pause=0;
			break;
		case KEY_NEXT_NOUP:
			pca->ph->pflag->exit=DEC_RET_NEXT_NOUP;
			pca->ph->pflag->pause=0;
		default:break;
	}	
	return 1;
}

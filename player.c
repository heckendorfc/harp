/*
 *  Copyright (C) 2009-2012  Christian Heckendorf <heckendorfc@gmail.com>
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
	int songid;
	int totaltime;
	int filetype;
	char location[250];
	struct pluginitem *pi_ptr;
};

static int initList(int list, char *query){
	// If we shuffled, TempPlaylistSong will be a temp table. list will be 0.
	if(list){ /* Should be unused */
		sprintf(query,"CREATE TEMP VIEW IF NOT EXISTS TempPlaylistSong AS SELECT PlaylistSongID, SongID, \"Order\" FROM PlaylistSong WHERE PlaylistID=%d ORDER BY \"Order\"",list);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		sprintf(query,"SELECT Song.SongID, \"Order\" FROM Song, PlaylistSong WHERE Song.SongID=PlaylistSong.SongID AND PlaylistSong.PlaylistID=%d ORDER BY \"Order\"",list);
	}
	else{
		sprintf(query,"SELECT Song.SongID,\"Order\" FROM Song, TempPlaylistSong WHERE Song.SongID=TempPlaylistSong.SongID ORDER BY \"Order\" LIMIT 1");
	}
	return 0;
}

static int play_handle_key(char *key){
	if(*key==KEY_QUIT)
		return 1;
	pthread_mutex_lock(&actkey);
		*key=KEY_NULL;
	pthread_mutex_unlock(&actkey);

	return 0;
}

static int play_handle_SongPubInfo(void *data, int col_count, char **row, char **titles){
	struct play_song_args *psargs=(struct play_song_args *)data;

	printSongPubInfo(row);

	psargs->totaltime=(int)strtol(row[5],NULL,10);
	psargs->ph->pflag->rating=(int)strtol(row[7],NULL,10);
	psargs->filetype=getFileTypeByName(row[4]);
	sprintf(psargs->location,"%s",row[1]);

	return 0;
}

static int play_handle_plugin(struct play_song_args *psargs){
	int ret=0;

	findPluginIDByType(0); // Reset
	while((psargs->pi_ptr=findPluginByID(psargs->ph->plugin_head,findPluginIDByType(psargs->filetype)))){
		if((psargs->ph->ffd=psargs->pi_ptr->modopen(psargs->location,"rb"))!=NULL){
			psargs->pca->decoder=psargs->pi_ptr;
			ret=psargs->pi_ptr->modplay(psargs->ph,psargs->pca->key,&psargs->totaltime);
			psargs->pi_ptr->modclose(psargs->ph->ffd);
			psargs->pca->decoder=NULL;
			break;
		}
		else{
			fprintf(stderr,"Failed to open file\n");
			return 0;
		}
	}
	if(!psargs->pi_ptr)
		ret=-1;

	return ret;
}

static void play_update_stats(struct play_song_args *psargs, int ret){
	char query[300];

	if(ret!=DEC_RET_ERROR){ // Update stats
		switch(ret){
			case DEC_RET_SUCCESS: // Normal play.
				if(psargs->totaltime>0)
					sprintf(query,"UPDATE Song SET PlayCount=PlayCount+1, LastPlay=%d, Length=%d WHERE SongID=%d",(int)time(NULL),psargs->totaltime,psargs->songid);
				else
					sprintf(query,"UPDATE Song SET PlayCount=PlayCount+1, LastPlay=%d, WHERE SongID=%d",(int)time(NULL),psargs->songid);
				break;
			case DEC_RET_NEXT: // Next key
				sprintf(query,"UPDATE Song SET SkipCount=SkipCount+1, LastPlay=%d WHERE SongID=%d",(int)time(NULL),psargs->songid);
				break;
			case DEC_RET_NEXT_NOUP: // Next key without update[playcount]
			default:
				sprintf(query,"UPDATE Song SET LastPlay=%d WHERE SongID=%d",(int)time(NULL),psargs->songid);
				break;
		}
		debug(3,query);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	sprintf(query,"UPDATE Song SET Rating=%d WHERE SongID=%d",psargs->ph->pflag->rating,psargs->songid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
}

void play_next_song_query(char *query, struct playercontrolarg *pca){
	sprintf(query,"SELECT Song.SongID,\"Order\" FROM Song NATURAL JOIN TempPlaylistSong WHERE \"Order\">=%d ORDER BY \"Order\" LIMIT 1",pca->next_order);
}

int play_song(void *data, int col_count, char **row, char **titles){
	struct play_song_args *psargs=(struct play_song_args *)data;
	int ret;
	char query[200];

	psargs->songid=(int)strtol(row[0],NULL,10);
	psargs->pca->next_order=psargs->pca->cur_order=strtol(row[1],NULL,10);
	psargs->pca->next_order++;

	sprintf(query,"SELECT * FROM SongPubInfo WHERE SongID=%d LIMIT 1",psargs->songid);
	if(harp_sqlite3_exec(conn,query,play_handle_SongPubInfo,psargs,NULL)!=SQLITE_OK)
		return 0;

	return 1;
}

static void setPause(int pause,struct playerflag *pflag){
	pflag->pause=pause;
	pflag->pausec=pause?'P':32;
}

static void printStatus(struct playerstatusarg *psa){
	fprintf(stdout,"\r [%c %c][%ds of %ds (%d%%)]%s", psa->pflag->pausec, psa->pflag->mutec, psa->outdetail->curtime, psa->outdetail->totaltime, psa->outdetail->percent<-1?-1:psa->outdetail->percent,psa->outdetail->tail);
	fflush(stdout);
}

static void playerStatus(void *arg){
	struct playerstatusarg *psa=(struct playerstatusarg*)arg;
	while(1){
		if(psa->pflag->update){
			printStatus(psa);
		}
		usleep(STATUS_REFRESH_INTERVAL);
	}
}

static void playerControl(void *arg){
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

int player(int list){//list - playlist number
	unsigned int max_list_order;
	int oldupdate;
	int ret;
	char *query; // Why aren't we using []?
	char library[255];
	
	// Create playerControl thread
	char key=KEY_NULL;
	struct play_song_args psargs;
	struct playercontrolarg pca;
	struct playerstatusarg psa;
	struct playerHandles ph;
	struct playerflag pflag={0,0,1,0,DEC_RET_SUCCESS,32,32};
	struct outputdetail outdetail;
	bzero(&outdetail,sizeof(outdetail));

	if(!(query=malloc(sizeof(char)*320))){
		debug(2,"Malloc failed (player query).");
		return 1;
	}
	else{
		char small_query[128];

		if(initList(list,query))
			return 0;

		sprintf(small_query,"SELECT MAX(\"Order\") FROM TempPlaylistSong");
		harp_sqlite3_exec(conn,small_query,uint_return_cb,&max_list_order,NULL);

		if(!(ph.plugin_head=openPlugins())){
			fprintf(stderr,"No plugins found. Please add them with harp -a\n");
			return 0;
		}
	}


	ph.ffd=NULL;
	ph.device=arglist[ADEVICE].subarg;
	ph.dechandle=NULL;
	psa.pflag=ph.pflag=&pflag;
	psa.outdetail=ph.outdetail=&outdetail;
	pca.ph=&ph;
	tcgetattr(0,&pca.orig);
	pca.key=&key;
	psargs.ph=&ph;
	psargs.pca=&pca;
	oldupdate=1;
	
	pthread_t control_thread,status_thread;
	pthread_mutex_init(&actkey,NULL);
	pthread_mutex_init(&outstatus,NULL);
	pthread_create(&control_thread,NULL,(void *)&playerControl,(void*)&pca);
	pthread_create(&status_thread,NULL,(void *)&playerStatus,(void*)&psa);
	
	// Play the list!
	if(snd_init(&ph)){
		fprintf(stderr,"snd_init failed");
		closePluginList(ph.plugin_head);
		free(query);
		return 1;
	}

	// Run the song playing query. Loop for jumping.
	while(harp_sqlite3_exec(conn,query,play_song,&psargs,NULL)==SQLITE_ABORT){
		bzero(&outdetail,sizeof(outdetail));
		psargs.ph->pflag->update=oldupdate;
		if((ret=play_handle_plugin(&psargs))<1){
			if(ret<0) /* No plugins */
				break;
			else{ /* Error opening file */
				if(play_handle_key(psargs.pca->key))
					break;
				play_next_song_query(query,&pca);
				continue;
			}
		}
		oldupdate=psargs.ph->pflag->update;
		psargs.ph->pflag->update=0;
		printf("\n");

		psargs.ph->ffd=NULL;
		psargs.ph->dechandle=NULL;
		psargs.ph->pflag->exit=DEC_RET_SUCCESS;

		play_update_stats(&psargs,ret);

		if(play_handle_key(psargs.pca->key))
			break;

		if(arglist[AREPEAT].active>0 &&
		  pca.next_order==pca.cur_order+1 && // Not jumping back
		  pca.next_order>max_list_order){
			pca.next_order=1;
			if(arglist[AREPEAT].subarg!=NULL)
				arglist[AREPEAT].active--;
		}

		play_next_song_query(query,&pca);
	}

	closePluginList(ph.plugin_head);
	snd_close(&ph);
	pthread_cancel(status_thread);
	pthread_cancel(control_thread);
	// Reset term settings
	tcsetattr(0,TCSANOW,&pca.orig);
	pthread_mutex_destroy(&outstatus);
	pthread_mutex_destroy(&actkey);
	printf("\rExiting.\n");
	free(query);
	return 0;
}

static void writelist_file(char *com, struct playercontrolarg *pca){
	int x,y,limit;
	unsigned int order;
	FILE *ffd;
	struct dbitem dbi;
	char query[200],filename[30];
	dbiInit(&dbi);

	for(y=1;y<ADV_COM_ARG_LEN && com[y] && (com[y]<'0' || com[y]>'9');y++);
	limit=(int)strtol(&com[y],NULL,10);
	if(limit<=0)limit=50;
	//for(x=1;x<y && com[x];y++);
	switch(com[1]){
		case 'h':
			sprintf(query,"SELECT \"Order\" AS \"#\",SongID,Title,Location,Rating,PlayCount,SkipCount,LastPlay FROM Song NATURAL JOIN TempPlaylistSong ORDER BY \"Order\" LIMIT %d",limit);
			break;
		case 't':
			harp_sqlite3_exec(conn,query,uint_return_cb,&order,NULL);
			if(x<0)x=0;
			sprintf(query,"SELECT \"Order\" AS \"#\",SongID,Title,Location,Rating,PlayCount,SkipCount,LastPlay FROM Song NATURAL JOIN TempPlaylistSong ORDER BY \"Order\" LIMIT %d",limit);
			break;
		case 'r':
		default:
			harp_sqlite3_exec(conn,query,uint_return_cb,&order,NULL);
			sprintf(query,"SELECT \"Order\" AS \"#\",SongID,Title,Location,Rating,PlayCount,SkipCount,LastPlay FROM Song NATURAL JOIN TempPlaylistSong ORDER BY \"Order\" LIMIT %d",limit);
			break;
	}
	sprintf(filename,"harp_stats_%d.csv",(int)time(NULL));
	if((ffd=fopen(filename,"w"))==NULL){
		fprintf(stderr,"Failed to open file\n");
		return;
	}
	fputs("ORDER\tID\tTITLE\tLOCATION\tRATING\tPLAYCOUNT\tSKIPCOUNT\tLASTPLAY\n",ffd);
	harp_sqlite3_exec(conn,query,write_stats_cb,ffd,NULL);
	fclose(ffd);
}

static void writelist_db(char *com, struct playercontrolarg *pca){
	char query[200];
	unsigned int pid=0;

	sprintf(query,"SELECT PlaylistID FROM Playlist WHERE Title='"SAVED_PLAYLIST_NAME"' LIMIT 1");
	harp_sqlite3_exec(conn,query,uint_return_cb,&pid,NULL);

	if(!pid){
		sprintf(query,SAVED_PLAYLIST_NAME);
		pid=getPlaylist(query);
	}

	sprintf(query,"DELETE FROM PlaylistSong WHERE PlaylistID=%d",pid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	sprintf(query,"INSERT INTO PlaylistSong (PlaylistID,SongID,\"Order\") SELECT %d,SongID,\"Order\" FROM TempPlaylistSong",pid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
}

static void writelist(char *com, struct playercontrolarg *pca){
	if(com[1]=='f')
		writelist_file(com,pca);
	else
		writelist_db(com,pca);
}

static void advseek(char *com, struct playercontrolarg *pca){
	int x,y;
	for(y=1;y<ADV_COM_ARG_LEN && com[y] && (com[y]<'0' || com[y]>'9');y++);
	int time=(int)strtol(&com[y],NULL,10);
	if(time<=0)return;
	
	if(com[0]==KEY_SEEK_DN)
		time*=-1;

	if(!pca->decoder)return;
	setPause(0,pca->ph->pflag);
	pca->decoder->modseek(pca->ph,time);
}

static void jump(char *com, struct playercontrolarg *pca){
	char query[200];
	int x,y,dest,max_dest;

	sprintf(query,"SELECT MAX(\"Order\") FROM TempPlaylistSong");
	harp_sqlite3_exec(conn,query,uint_return_cb,&max_dest,NULL);

	for(y=1;y<ADV_COM_ARG_LEN && com[y] && (com[y]<'0' || com[y]>'9');y++);
	if((dest=(int)strtol(&com[y],NULL,10))<=0)return;

	if(com[1]=='s'){ // Jump by SongID. Find the Order first,
		sprintf(query,"SELECT \"Order\" FROM TempPlaylistSong WHERE SongID=%d",dest);
		debug(3,query);
		dest=0;
		harp_sqlite3_exec(conn,query,uint_return_cb,&dest,NULL);
	}
	if(dest<=0 || dest>max_dest)return;

	pca->next_order=dest;

	pthread_mutex_lock(&actkey);
		*pca->key=(*com=='j')?KEY_NEXT:KEY_NEXT_NOUP;
	pthread_mutex_unlock(&actkey);
	pca->ph->pflag->exit=*pca->key;
	setPause(0,pca->ph->pflag);
}

static void listtemp(char *com, struct playercontrolarg *pca){
	char query[200];
	int y,limit,exception[10];
	unsigned int order;
	for(y=2;y<10;y++)exception[y]=listconf.exception;
	exception[0]=exception[1]=1;

	for(y=1;y<ADV_COM_ARG_LEN && com[y] && (com[y]<'0' || com[y]>'9');y++);
	limit=(int)strtol(&com[y],NULL,10);
	if(limit<=0)limit=30;
	switch(com[1]){
		case 'h': // Head
			sprintf(query,"SELECT \"Order\" AS \"#\",SongID,SongTitle,AlbumTitle,ArtistName FROM TempPlaylistSong NATURAL JOIN SongPubInfo ORDER BY \"Order\" LIMIT %d",limit);
			break;
		case 't': // Tail
			harp_sqlite3_exec(conn,"SELECT MAX(\"Order\") FROM TempPlaylistSong",uint_return_cb,&order,NULL);
			sprintf(query,"SELECT \"Order\" AS \"#\",SongID,SongTitle,AlbumTitle,ArtistName FROM TempPlaylistSong NATURAL JOIN SongPubInfo WHERE \"Order\">%d ORDER BY \"Order\" LIMIT %d",order-limit,limit);
			break;
		case 'r': // Relative
		default:
			sprintf(query,"SELECT \"Order\" AS \"#\",SongID,SongTitle,AlbumTitle,ArtistName FROM TempPlaylistSong NATURAL JOIN SongPubInfo WHERE \"Order\">=%d ORDER BY \"Order\" LIMIT %d",pca->cur_order,limit);
			break;
	}
	debug(3,query);
	doTitleQuery(query,exception,listconf.maxwidth);
}


static int remitem_order_shift_cb(void *arg, int col_count, char **row, char **titles){
	struct playercontrolarg *pca=(struct playercontrolarg *)arg;
	char query[150];
	int order=(int)strtol(*row,NULL,10);
	if(order>0){
		if(pca->cur_order>=order)pca->cur_order--;
		if(pca->next_order>=order)pca->next_order--;
	}
	sprintf(query,"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"-1 WHERE \"Order\">=%s",*row);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	return 0;
}

static void remitem(char *com, struct playercontrolarg *pca){
	char query[250],cb_query[250];
	int x,order=0;
	struct IDList id_struct;

	if(com[1]=='o'){
		for(x=2;x<ADV_COM_ARG_LEN && com[x] && (com[x]<'0' || com[x]>'9');x++);
		if((com[x]<'0' && com[x]>'9') || (order=(int)strtol(com+x,NULL,10))<1){
			fprintf(stderr,"Improper command format\n");
			return;
		}
		sprintf(query,"DELETE FROM TempPlaylistSong WHERE \"Order\"=%d",order);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		printf("Removed %d songs.\n",sqlite3_changes(conn));
		sprintf(query,"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"-1 WHERE \"Order\">%d",order);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		if(pca->cur_order>order)pca->cur_order--;
		if(pca->next_order>order)pca->next_order--;
	}
	else{
		for(x=2;x<ADV_COM_ARG_LEN && com[x] && com[x]==' ';x++);
		if(getGroupSongIDs(com+x,ADV_COM_ARG_LEN,&id_struct)==HARP_RET_ERR)
			return;

		sprintf(query,"SELECT DISTINCT \"Order\",SongID FROM TempPlaylistSong WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d) ORDER BY \"Order\" DESC",id_struct.tempselectid);
		harp_sqlite3_exec(conn,query,remitem_order_shift_cb,pca,NULL);

		sprintf(query,"DELETE FROM TempPlaylistSong WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",id_struct.tempselectid);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

		printf("Removed %d songs.\n",id_struct.length);
		cleanTempSelect(id_struct.tempselectid);
		free(id_struct.songid);
	}
}

static void additem(char *com, struct playercontrolarg *pca){
	struct IDList id_struct;
	int x=1;
	int dest=0;
	char query[150],cb_query[150];
	struct insert_tps_arg data;

	if(com[1]=='o'){
		for(x=2;x<ADV_COM_ARG_LEN && com[x] && com[x]==' ';x++);
		if((com[x]<'0' && com[x]>'9') || (dest=(int)strtol(com+x,NULL,10))<1){
			fprintf(stderr,"Improper command format\n");
			return;
		}
		for(;x<ADV_COM_ARG_LEN && com[x] && com[x]>='0' && com[x]<='9';x++);
	}

	for(;x<ADV_COM_ARG_LEN && com[x] && com[x]==' ';x++);

	if(getGroupSongIDs(com+x,ADV_COM_ARG_LEN,&id_struct)==HARP_RET_ERR)
		return;

	if(dest<1)
		dest=pca->next_order;

	sprintf(query,"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"+%d WHERE \"Order\">=%d",id_struct.length,dest);
	printf("Added %d songs starting at order %d.\n",id_struct.length,dest);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	if(pca->cur_order>=dest)pca->cur_order+=id_struct.length;
	if(pca->next_order>dest)pca->next_order+=id_struct.length;

	data.query=cb_query;
	data.order=dest;
	data.count=0;
	sprintf(query,"SELECT SelectID FROM TempSelect WHERE TempID=%d",id_struct.tempselectid);
	harp_sqlite3_exec(conn,query,batch_tempplaylistsong_insert_cb,&data,NULL);

	cleanTempSelect(id_struct.tempselectid);
	free(id_struct.songid);
}

struct adv_list_item{
	char key;
	void (*func)(char *, struct playercontrolarg *);
}adv_list[]={
	{'l',listtemp},// List the playlist
	{'w',writelist},// Write out information about the playlist
	{'j',jump},// Jump to a different song
	{'J',jump},// Jump to a different song
	{KEY_SEEK_DN,advseek},// Seek with specified value
	{KEY_SEEK_UP,advseek},// Seek with specified value
	{'a',additem},
	{'r',remitem},
	{0,NULL}
};

static void getCommand(struct playercontrolarg *pca){
	struct adv_list_item *list=adv_list;
	struct termios old;
	char *ptr,com[ADV_COM_ARG_LEN];
	int x,oldupdate=pca->ph->pflag->update;
	struct playerstatusarg psa={pca->ph->pflag,pca->ph->outdetail};

	tcsetattr(0,TCSANOW,&pca->orig);
	bzero(com,ADV_COM_ARG_LEN);
	ptr=com;
	psa.pflag->update=0;
	pthread_mutex_lock(&outstatus);
		for(x=0;psa.outdetail->tail[x];x++)psa.outdetail->tail[x]=' ';
	pthread_mutex_unlock(&outstatus);
	printStatus(&psa);
	pthread_mutex_lock(&outstatus);
		sprintf(psa.outdetail->tail,":");
	pthread_mutex_unlock(&outstatus);
	printStatus(&psa);

	if(fgets(com,ADV_COM_ARG_LEN,stdin)){
		int x;

		for(x=0;x<ADV_COM_ARG_LEN && com[x]!='\n' && com[x];x++);
		if(x<ADV_COM_ARG_LEN)com[x]=0;

		while(list->key){
			if(*com==list->key)
				list->func(ptr,pca);
			list++;
		}
	}
	tcgetattr(0,&old);
	old.c_lflag&=(~ICANON);
	old.c_lflag&=(~ECHO);
	old.c_cc[VTIME]=0;
	old.c_cc[VMIN]=1;
	tcsetattr(0,TCSANOW,&old);
	pthread_mutex_lock(&outstatus);
		sprintf(psa.outdetail->tail,"\r");
	pthread_mutex_unlock(&outstatus);
	pca->ph->pflag->update=oldupdate;
}

int getSystemKey(char key, struct playercontrolarg *pca){
	function_seek seek;
	int oldrating;
	char tail[OUTPUT_TAIL_SIZE];
	switch(key){
		case KEY_VOLUP:
			changeVolume(pca->ph,5);
			break;
		case KEY_VOLDN:
			changeVolume(pca->ph,-5);
			break;
		case KEY_MUTE:
			toggleMute(pca->ph,&pca->ph->pflag->mute);
			pca->ph->pflag->mutec=pca->ph->pflag->mutec==32?'M':32;
			break;
		case KEY_PAUSE: 
			setPause(!pca->ph->pflag->pause,pca->ph->pflag);
			break;
		case KEY_PREV:
			if(!pca->decoder)break;
			setPause(0,pca->ph->pflag);
			pca->decoder->modseek(pca->ph,0);
			break;
		case KEY_SEEK_UP:
			if(!pca->decoder)break;
			setPause(0,pca->ph->pflag);
			pca->decoder->modseek(pca->ph,20);
			break;
		case KEY_SEEK_DN:
			if(!pca->decoder)break;
			setPause(0,pca->ph->pflag);
			pca->decoder->modseek(pca->ph,-20);
			break;
		case KEY_RATEUP:
			oldrating=pca->ph->pflag->rating;
			pca->ph->pflag->rating=pca->ph->pflag->rating==10?10:pca->ph->pflag->rating+1;
			pthread_mutex_lock(&outstatus);
				sprintf(tail,"Rating: %d/10 -> %d/10",oldrating,pca->ph->pflag->rating);
			pthread_mutex_unlock(&outstatus);
			addStatusTail(tail,pca->ph->outdetail);
			break;
		case KEY_RATEDN:
			oldrating=pca->ph->pflag->rating;
			pca->ph->pflag->rating=pca->ph->pflag->rating==0?0:pca->ph->pflag->rating-1;
			pthread_mutex_lock(&outstatus);
				sprintf(tail,"Rating: %d/10 -> %d/10",oldrating,pca->ph->pflag->rating);
			pthread_mutex_unlock(&outstatus);
			addStatusTail(tail,pca->ph->outdetail);
			break;
		case KEY_COMMAND:
			getCommand(pca);
			break;
		case KEY_QUIT:
			pca->ph->pflag->exit=DEC_RET_ERROR;
			setPause(0,pca->ph->pflag);
			return 0;
		case KEY_NEXT:
			pca->ph->pflag->exit=DEC_RET_NEXT;
			setPause(0,pca->ph->pflag);
			break;
		case KEY_NEXT_NOUP:
			pca->ph->pflag->exit=DEC_RET_NEXT_NOUP;
			setPause(0,pca->ph->pflag);
		default:break;
	}	
	return 1;
}

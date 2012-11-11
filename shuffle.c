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

#include "shuffle.h"
#include "defs.h"
#include "dbact.h"
#include "util.h"

struct shuffle_data{
	int count;
	int batch_count;
	int tail;
	char *query;
};

static struct shuffle_queries{
	char group;
	char *count;
	char *primary_select;
	char *fill;
}shuffle_q[]={
	{	's',
		"SELECT COUNT(SelectID) FROM TempSelect WHERE TempID=%d",
		"SELECT SelectID FROM TempSelect WHERE TempID=%d",
		"SELECT SongID,\"Order\",%d FROM TempPlaylistSong WHERE \"Order\"<0 ORDER BY \"Order\" ASC"
		//"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"*-1"
	},
	{	'a',
		"SELECT COUNT(SongID) FROM Song WHERE SongID IN (SELECT SelectID FROM TempSelect,Song WHERE SelectID=SongID AND TempID=%d GROUP BY AlbumID)",
		"SELECT DISTINCT AlbumID FROM Song WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",
		"SELECT SelectID,\"Order\" FROM TempSelect INNER JOIN Song ON SelectID=Song.SongID INNER JOIN TempPlaylistSong ON TempPlaylistSong.SongID=Song.AlbumID WHERE TempID=%d AND \"Order\"<1 ORDER BY \"Order\" ASC,Track ASC"
	},
	{	'r',
		"SELECT COUNT(SongID) FROM Song WHERE SongID IN (SELECT SelectID FROM TempSelect,Song,AlbumArtist WHERE SelectID=SongID AND Song.AlbumID=AlbumArtist.AlbumID AND TempID=%d GROUP BY ArtistID)",
		"SELECT DISTINCT ArtistID FROM AlbumArtist NATURAL JOIN Song WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",
		"SELECT SelectID,\"Order\" FROM TempSelect INNER JOIN Song ON SelectID=Song.SongID NATURAL JOIN AlbumArtist INNER JOIN TempPlaylistSong ON TempPlaylistSong.SongID=ArtistID WHERE TempID=%d AND \"Order\"<1 ORDER BY \"Order\" ASC,AlbumID ASC,Track ASC"
	},
	{0,NULL,NULL,NULL}
};

static void fillTempPlaylistSong(int tempid,int group){
	char query[350],cb_query[150];
	static struct insert_tps_arg data={1,0,NULL};
	data.query=cb_query;
	sprintf(query,shuffle_q[group].fill,tempid);
	harp_sqlite3_exec(conn,query,batch_tempplaylistsong_insert_cb,&data,NULL);
}

static int rand_next(struct shuffle_data *data){
	return random()%data->count;
}

static int clean_shuffle_cb(void *arg, int col_count, char **row, char **titles){
	unsigned int order;
	struct shuffle_data *data=(struct shuffle_data*)arg;
	sprintf(data->query,"UPDATE TempPlaylistSong SET \"Order\"=%d WHERE SongID=%s",-(data->tail++),*row);
	harp_sqlite3_exec(conn,data->query,NULL,NULL,NULL);
	data->batch_count++;
	if(data->count>DB_BATCH_SIZE){
		data->batch_count=0;
		harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
		harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	}
	return 0;
}

static int shuffle_cb(void *arg, int col_count, char **row, char **titles){
	unsigned int order;
	struct shuffle_data *data=(struct shuffle_data*)arg;
	if((order=rand_next(data))==-1){
		debug(2,"rand_next returned -1");
		return 1;
	}
	sprintf(data->query,"INSERT INTO TempPlaylistSong(SongID,\"Order\") VALUES(%s,%d)",*row,-(order+1));
	harp_sqlite3_exec(conn,data->query,NULL,NULL,NULL);
	data->batch_count++;
	if(data->count>DB_BATCH_SIZE){
		data->batch_count=0;
		harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
		harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	}
	return 0;
}

void shuffle(int list){
	char query[350],cb_query[350];
	int tempid,group=0;
	unsigned int items=0;
	struct shuffle_data data;

	srandom((unsigned int)time(NULL));
	
	if(arglist[ASHUFFLE].active && arglist[ASHUFFLE].subarg){
		for(group=0;shuffle_q[group].group && shuffle_q[group].group!=*arglist[ASHUFFLE].subarg;group++);
		if(!shuffle_q[group].group)
			group=0;
	}

	if(list){
		sprintf(query,"SELECT %%d,SongID FROM PlaylistSong WHERE PlaylistID=%d",list);
		tempid=insertTempSelectQuery(query);
		createTempPlaylistSong();
	}
	else{
		tempid=insertTempSelectQuery("SELECT %d,SongID FROM TempPlaylistSong");
	}
	
	sprintf(query,shuffle_q[group].count,tempid);
	harp_sqlite3_exec(conn,query,uint_return_cb,&items,NULL);
	sprintf(query,shuffle_q[group].primary_select,tempid);
	
	harp_sqlite3_exec(conn,"DELETE FROM TempPlaylistSong",NULL,NULL,NULL);

	data.count=items;
	data.batch_count=0;
	data.query=cb_query;
	harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	harp_sqlite3_exec(conn,query,shuffle_cb,&data,NULL);
	harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);

	data.count=items;
	data.batch_count=0;
	data.tail=items+1;
	sprintf(query,"SELECT b.SongID,b.SongID FROM TempPlaylistSong a, TempPlaylistSong b WHERE a.SongID<b.SongID AND a.\"Order\"=b.\"Order\" GROUP BY b.SongID ORDER BY \"Order\"");
	harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	harp_sqlite3_exec(conn,query,clean_shuffle_cb,&data,NULL);
	harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);

	fillTempPlaylistSong(tempid,group);

	harp_sqlite3_exec(conn,"DELETE FROM TempPlaylistSong WHERE \"Order\"<1",NULL,NULL,NULL); /* Just in case */
	if(arglist[AZSHUFFLE].active){
		debug(1,"Z Shuffling");
		zshuffle(items);
	}
}

struct zs_arg{
	const unsigned int items;
	const unsigned int group_items;
	const unsigned int increment;
	union{
		const unsigned int skip;
		const unsigned int slide;
	};
	unsigned int count;
	int slidemod;
	char *query;
};

static int zs_skip_cb(void *arg, int col_count, char **row, char **titles){
	struct zs_arg *data = (struct zs_arg*)arg;
	int id=data->count;

	// SWAP ORDERS!
	sprintf(data->query,"UPDATE TempPlaylistSong SET \"Order\"=%s  WHERE \"Order\"=%d",*(row+2),id);
	harp_sqlite3_exec(conn,data->query,NULL,NULL,NULL);
	sprintf(data->query,"UPDATE TempPlaylistSong SET \"Order\"=%d  WHERE PlaylistSongID=%s",id,*(row+1));
	harp_sqlite3_exec(conn,data->query,NULL,NULL,NULL);

	data->count+=data->increment;
	return 0;
}

static int zs_slide_cb(void *arg, int col_count, char **row, char **titles){
	struct zs_arg *data = (struct zs_arg*)arg;
	int order=(int)strtol(*(row+2),NULL,10);
	int stat;
	if(col_count>3)
		stat=(int)strtol(*(row+3),NULL,10);
	else
		stat=1;
	int neworder=order+(data->slidemod*stat);
	if(neworder<1)neworder=1;
	else if(neworder>data->items)neworder=data->items;
	
	if(data->slidemod<0) // Moving up
		sprintf(data->query,"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"+1  WHERE \"Order\">%d AND \"Order\"<(SELECT \"Order\" FROM TempPlaylistSong WHERE PlaylistSongID=%s LIMIT 1)",neworder-1,*(row+1));
	else // Moving down
		sprintf(data->query,"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"-1  WHERE \"Order\"<%d AND \"Order\">(SELECT \"Order\" FROM TempPlaylistSong WHERE PlaylistSongID=%s LIMIT 1)",neworder+1,*(row+1));
	harp_sqlite3_exec(conn,data->query,NULL,NULL,NULL);
	sprintf(data->query,"UPDATE TempPlaylistSong SET \"Order\"=%d  WHERE PlaylistSongID=%s",neworder,*(row+1));
	harp_sqlite3_exec(conn,data->query,NULL,NULL,NULL);

	data->count++;
	if(data->count>DB_BATCH_SIZE){
		data->count=0;
		harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
		harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	}
	return 0;
}

void zshuffle(unsigned int items){
	char query[250],cb_query[250];
	int x;
	const int ten_percent=items*0.1;
	const int alter_limit=100; // Max songs to alter
	const int mod_count=ten_percent>alter_limit?alter_limit:ten_percent; // Songs to alter this round
	const int group=(mod_count>>2)|1; // Number of songs to float each iter

	srandom((unsigned int)time(NULL));
	struct zs_arg data={items,group,(random()%2)+2,(random()%3)+2,0,1,cb_query};

	sprintf(query,"Mod count: %d\nGroup: %d\nIncrement: %d",mod_count,group,data.increment);
	debug(2,query);

	// Skip Count
	data.slidemod=data.slide*3;
	data.count=0;
	sprintf(query,"SELECT Song.SongID,PlaylistSongID,\"Order\" FROM TempPlaylistSong NATURAL JOIN Song WHERE SkipCount>0 ORDER BY SkipCount DESC LIMIT %d",mod_count);
	harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	harp_sqlite3_exec(conn,query,zs_slide_cb,&data,NULL);
	harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);

	// Play Count
	data.slidemod=data.slide*-2;
	data.count=0;
	sprintf(query,"SELECT Song.SongID,PlaylistSongID,\"Order\" FROM TempPlaylistSong NATURAL JOIN Song WHERE PlayCount>0 ORDER BY SkipCount DESC LIMIT %d",mod_count);
	harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	harp_sqlite3_exec(conn,query,zs_slide_cb,&data,NULL);
	harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);

	// Rating
	data.slidemod=data.slide*-1;
	data.count=0;
	sprintf(query,"SELECT Song.SongID,PlaylistSongID,\"Order\",Rating FROM TempPlaylistSong NATURAL JOIN Song WHERE Rating>3 ORDER BY SkipCount DESC LIMIT %d",mod_count);
	harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	harp_sqlite3_exec(conn,query,zs_slide_cb,&data,NULL);
	harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);

	// Last Play
	data.count=data.increment;
	sprintf(query,"SELECT COUNT(PlaylistSongID) FROM TempPlaylistSong NATURAL JOIN Song WHERE LastPlay=0");
	harp_sqlite3_exec(conn,query,uint_return_cb,&x,NULL);
	if(x>ten_percent){
		debug(1,"ZSHUFFLE | Too many unplayed songs; skipping LastPlay modifier.");
		return;
	}
	for(x=0;x<mod_count;x+=group){
		// What have I done...
		sprintf(query,"SELECT SongID,PlaylistSongID,\"Order\" FROM TempPlaylistSong WHERE SongID IN (SELECT SongID FROM Song NATURAL JOIN TempPlaylistSong ORDER BY LastPlay ASC LIMIT %d,%d) ORDER BY \"Order\" ASC",x,group);
		harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
		harp_sqlite3_exec(conn,query,zs_skip_cb,&data,NULL);
		harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
	}
}


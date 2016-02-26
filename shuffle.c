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
		"SELECT SelectID,1 FROM TempSelect WHERE TempID=%d",
		"SELECT SongID,%d FROM TempPlaylistSong WHERE \"Order\"<0 ORDER BY \"Order\" ASC"
		//"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"*-1"
	},
	{	'a',
		"SELECT COUNT(SongID) FROM Song WHERE SongID IN (SELECT SelectID FROM TempSelect,Song WHERE SelectID=SongID AND TempID=%d GROUP BY AlbumID)",
		"SELECT DISTINCT AlbumID,1 FROM Song WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",
		"SELECT SelectID,1 FROM TempSelect INNER JOIN Song ON SelectID=Song.SongID INNER JOIN TempPlaylistSong ON TempPlaylistSong.SongID=Song.AlbumID WHERE TempID=%d AND \"Order\"<1 ORDER BY \"Order\" ASC,Track ASC"
	},
	{	'r',
		"SELECT COUNT(SongID) FROM Song WHERE SongID IN (SELECT SelectID FROM TempSelect,Song,AlbumArtist WHERE SelectID=SongID AND Song.AlbumID=AlbumArtist.AlbumID AND TempID=%d GROUP BY ArtistID)",
		"SELECT DISTINCT ArtistID,1 FROM AlbumArtist NATURAL JOIN Song WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",
		"SELECT SelectID,1 FROM TempSelect INNER JOIN Song ON SelectID=Song.SongID NATURAL JOIN AlbumArtist INNER JOIN TempPlaylistSong ON TempPlaylistSong.SongID=ArtistID WHERE TempID=%d AND \"Order\"<1 ORDER BY \"Order\" ASC,AlbumID ASC,Track ASC"
	},
	{0,NULL,NULL,NULL}
};

static void fillTempPlaylistSong(int tempid,int group){
	char query[450],cb_query[450];

	harp_sqlite3_exec(conn,"CREATE TEMPORARY TRIGGER ShuffleList AFTER INSERT ON TempPlaylistSong BEGIN "
			"UPDATE TempPlaylistSong SET \"Order\"=(SELECT count(PlaylistSongID) FROM TempPlaylistSong WHERE \"Order\">0) WHERE PlaylistSongID=NEW.PlaylistSongID;"
			"END",NULL,NULL,NULL);

	snprintf(cb_query,450,shuffle_q[group].fill,tempid);
	snprintf(query,450,"INSERT INTO TempPlaylistSong(SongID,\"Order\") %s",cb_query);
	debug(3,query);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	//harp_sqlite3_exec(conn,query,batch_tempplaylistsong_insert_cb,&data,NULL);

	harp_sqlite3_exec(conn,"DROP TRIGGER ShuffleList",NULL,NULL,NULL);
}

/*
static int rand_next(struct shuffle_data *data){
	return random()%data->count;
}

static int clean_shuffle_cb(void *arg, int col_count, char **row, char **titles){
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
*/

void shuffle(int list){
	char query[350],cb_query[350];
	int tempid,group=0;
	unsigned int items=0;

	srandom((unsigned int)time(NULL));

	if(arglist[ASHUFFLE].active && arglist[ASHUFFLE].subarg){
		for(group=0;shuffle_q[group].group && shuffle_q[group].group!=*arglist[ASHUFFLE].subarg;group++);
		if(!shuffle_q[group].group){
			debug(2,"Defaulting shuffle group to 's'");
			group=0;
		}
	}

	if(list){
		snprintf(query,350,"SELECT %%d,SongID FROM PlaylistSong WHERE PlaylistID=%d",list);
		tempid=insertTempSelectQuery(query);
		createTempPlaylistSong();
	}
	else{
		tempid=insertTempSelectQuery("SELECT %d,SongID FROM TempPlaylistSong");
	}

	snprintf(query,350,shuffle_q[group].count,tempid);
	harp_sqlite3_exec(conn,query,uint_return_cb,&items,NULL);
	snprintf(query,350,shuffle_q[group].primary_select,tempid);

	harp_sqlite3_exec(conn,"DELETE FROM TempPlaylistSong",NULL,NULL,NULL);

	harp_sqlite3_exec(conn,"CREATE TEMPORARY TRIGGER ShuffleList AFTER INSERT ON TempPlaylistSong BEGIN "
			"UPDATE TempPlaylistSong SET \"Order\"=(SELECT -count(PlaylistSongID) FROM TempPlaylistSong) WHERE PlaylistSongID=NEW.PlaylistSongID;"
			"END",NULL,NULL,NULL);

	snprintf(cb_query,350,"INSERT INTO TempPlaylistSong(SongID,\"Order\") %s ORDER BY random()",shuffle_q[group].primary_select);
	snprintf(query,350,cb_query,tempid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	harp_sqlite3_exec(conn,"DROP TRIGGER ShuffleList",NULL,NULL,NULL);

	harp_sqlite3_exec(conn,"DELETE FROM TempPlaylistSong WHERE \"Order\">0",NULL,NULL,NULL); /* Just in case */
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
	const unsigned int slide;
	unsigned int count;
	int slidemod;
	char *query;
};

struct candidate_data{
	int *list;
	int num;
};

static int zs_candidate_cb(void *arg, int col_count, char **row, char **titles){
	struct candidate_data *data = (struct candidate_data*)arg;

	data->list[data->num++]=strtol(row[0],NULL,10);

	return 0;
}

/*
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
*/

static void update_order_zs(struct candidate_data *candlist, int slidemod){
	char query[250];

	//harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	while(candlist->num>0){
		snprintf(query,250,"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"%+d WHERE PlaylistSongID=%d",slidemod,candlist->list[--candlist->num]);
		debug(3,query);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	//harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
}

static void skip_order_zs(struct candidate_data *candlist, int slidemod){
	char query[250];

	//harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	while(candlist->num>0){
		candlist->num--;
		snprintf(query,250,"UPDATE TempPlaylistSong SET \"Order\"=%d WHERE PlaylistSongID=%d",candlist->num+1,candlist->list[candlist->num]);
		harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	}
	//harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
}

void zshuffle(unsigned int items){
	char query[450],cb_query[250];
	int x;
	const int ten_percent=items*0.1;
	const int alter_limit=100; // Max songs to alter
	const int mod_count=ten_percent>alter_limit?alter_limit:ten_percent; // Songs to alter this round
	const int group=(mod_count>>2)|1; // Number of songs to float each iter
	struct candidate_data candlist;

	srandom((unsigned int)time(NULL));
	struct zs_arg data={.items=items,.group_items=group,.increment=(random()%2)+2,.slide=(random()%3)+2,.count=0,.slidemod=1,.query=cb_query};

	snprintf(query,450,"Mod count: %d\nGroup: %d\nIncrement: %d",mod_count,group,data.increment);
	debug(2,query);

	candlist.list=malloc(sizeof(*candlist.list)*mod_count);
	if(candlist.list==NULL)
		return;

	// Skip Count
	data.slidemod=data.slide*3;
	data.count=0;
	snprintf(query,450,"CREATE TEMPORARY TRIGGER SlideList AFTER UPDATE ON TempPlaylistSong BEGIN "
			"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"-1  WHERE \"Order\"<=NEW.\"Order\" AND \"Order\">=OLD.\"Order\" AND PlaylistSongID!=OLD.PlaylistSongID; "
			"END");
	x=harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	snprintf(query,450,"SELECT PlaylistSongID FROM TempPlaylistSong NATURAL JOIN Song WHERE SkipCount>0 AND \"Order\"<(SELECT COUNT(PlaylistSongID)-%d FROM TempPlaylistSong) ORDER BY SkipCount DESC LIMIT %d",data.slidemod,mod_count);
	candlist.num=0;
	harp_sqlite3_exec(conn,query,zs_candidate_cb,&candlist,NULL);
	snprintf(query,450,"Skip count mod count: %d",candlist.num);
	debug(2,query);
	update_order_zs(&candlist,data.slidemod);
	harp_sqlite3_exec(conn,"DROP TRIGGER SlideList",NULL,NULL,NULL);


	// Play Count
	data.slidemod=data.slide*-2;
	data.count=0;
	snprintf(query,450,"CREATE TEMPORARY TRIGGER SlideList AFTER UPDATE ON TempPlaylistSong BEGIN "
			"UPDATE TempPlaylistSong SET \"Order\"=\"Order\"+1  WHERE \"Order\">=NEW.\"Order\" AND \"Order\"<=OLD.\"Order\" AND PlaylistSongID!=OLD.PlaylistSongID; "
			"END");
	x=harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	snprintf(query,450,"SELECT PlaylistSongID FROM TempPlaylistSong NATURAL JOIN Song WHERE PlayCount>0 AND \"Order\">%d ORDER BY SkipCount DESC LIMIT %d",-data.slidemod,mod_count);
	candlist.num=0;
	harp_sqlite3_exec(conn,query,zs_candidate_cb,&candlist,NULL);
	snprintf(query,450,"Play count mod count: %d",candlist.num);
	debug(2,query);
	update_order_zs(&candlist,data.slidemod);

	// Rating
	data.slidemod=data.slide*-1;
	data.count=0;
	snprintf(query,450,"SELECT PlaylistSongID FROM TempPlaylistSong NATURAL JOIN Song WHERE Rating>3 AND \"Order\">%d ORDER BY SkipCount DESC LIMIT %d",-data.slidemod,mod_count);
	candlist.num=0;
	harp_sqlite3_exec(conn,query,zs_candidate_cb,&candlist,NULL);
	snprintf(query,450,"Rating mod count: %d",candlist.num);
	debug(2,query);
	update_order_zs(&candlist,data.slidemod);
	harp_sqlite3_exec(conn,"DROP TRIGGER SlideList",NULL,NULL,NULL);

	// Last Play
	data.count=data.increment;
	snprintf(query,450,"SELECT COUNT(PlaylistSongID) FROM TempPlaylistSong NATURAL JOIN Song WHERE LastPlay=0");
	harp_sqlite3_exec(conn,query,uint_return_cb,&x,NULL);
	if(x>ten_percent){
		debug(1,"ZSHUFFLE | Too many unplayed songs; skipping LastPlay modifier.");
		free(candlist.list);
		return;
	}
	snprintf(query,450,"CREATE TEMPORARY TRIGGER SkipList BEFORE UPDATE ON TempPlaylistSong BEGIN "
			"UPDATE TempPlaylistSong SET \"Order\"=OLD.\"Order\"  WHERE \"Order\"=NEW.\"Order\"; "
			"END");
	x=harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
	snprintf(query,450,"SELECT PlaylistSongID FROM TempPlaylistSong WHERE SongID IN (SELECT SongID FROM Song NATURAL JOIN TempPlaylistSong ORDER BY LastPlay ASC LIMIT %d) ORDER BY \"Order\" ASC",mod_count);
	candlist.num=0;
	harp_sqlite3_exec(conn,query,zs_candidate_cb,&candlist,NULL);
	skip_order_zs(&candlist,data.slidemod);
	harp_sqlite3_exec(conn,"DROP TRIGGER SkipList",NULL,NULL,NULL);

	free(candlist.list);
}


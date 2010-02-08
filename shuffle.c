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

#include "shuffle.h"
#include "defs.h"
#include "dbact.h"

const unsigned int primes[] = {
	2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79,
	83, 89, 97, 103, 109, 113, 127, 137, 139, 149, 157, 167, 179, 193, 199, 211, 227, 241, 257,
	277, 293, 313, 337, 359, 383, 409, 439, 467, 503, 541, 577, 619, 661, 709, 761, 823, 887,
	953, 1031, 1109, 1193, 1289, 1381, 1493, 1613, 1741, 1879, 2029, 2179, 2357, 2549, 2753, 2971,
	3209, 3469, 3739, 4027, 4349, 4703, 5087, 5503, 5953, 6427, 6949, 7517, 8123, 8783, 9497, 10273,
	11113, 12011, 12983, 14033, 15173, 16411, 17749, 19183, 20753, 22447, 24281, 26267, 28411, 30727,
	33223, 35933, 38873, 42043, 45481, 49201, 53201, 57557, 62233, 67307, 72817, 78779, 85229, 92203,
	99733, 107897, 116731, 126271, 136607, 147793, 159871, 172933, 187091, 202409, 218971, 236897,
	256279, 277261, 299951, 324503, 351061, 379787, 410857, 444487, 480881, 520241, 562841, 608903,
	658753, 712697, 771049, 834181, 902483, 976369, 1056323, 1142821, 1236397, 1337629, 1447153, 1565659,
	1693859, 1832561, 1982627, 2144977, 2320627, 2510653, 2716249, 2938679, 3179303, 3439651, 3721303, 4026031,
	4355707, 4712381, 5098259, 5515729, 5967347, 6456007, 6984629, 7556579, 8175383, 8844859, 9569143, 10352717,
	11200489, 12117689, 13109983, 14183539, 15345007, 16601593, 17961079, 19431899, 21023161, 22744717,
	24607243, 26622317, 28802401, 31160981, 33712729, 36473443, 39460231, 42691603, 46187573, 49969847,
	54061849, 58488943, 63278561, 68460391, 74066549, 80131819, 86693767, 93793069, 101473717, 109783337,
	118773397, 128499677, 139022417, 150406843, 162723577, 176048909, 190465427, 206062531, 222936881, 241193053,
	260944219, 282312799, 305431229, 330442829, 357502601, 386778277, 418451333, 452718089, 489790921, 529899637,
	573292817, 620239453, 671030513, 725980837, 785430967, 849749479, 919334987, 994618837, 1076067617, 1164186217,
	1259520799u, 1362662261u, 1474249943u, 1594975441u, 1725587117u, 1866894511u, 2019773507u, 2185171673u,
	2364114217u, 2557710269u, 2767159799u, 2993761039u, 3238918481u, 3504151727u, 3791104843u, 4101556399u, 4294967291u
};


struct prime_rand_data{
	unsigned int items;
	unsigned int prime;
	unsigned int searches;
	unsigned int current;
	unsigned int skip;
	unsigned int count;
	char *query;
};

static int prime_rand_next(struct prime_rand_data *data){
	if(data->searches==data->prime)
		return -1;
	int next=data->current;
	while(1){
		next+=data->skip;
		next%=data->prime;
		data->searches++;
		if(next<data->items){
			data->current=next;
			break;
		}
	}
	return data->current;
}

static struct prime_rand_data *prime_rand_init(int items){
	if(!items)return NULL;
	srandom((unsigned int)time(NULL));

	const unsigned int *nextprime=primes;
	while(*nextprime<items)
		nextprime++;

	struct prime_rand_data *data=malloc(sizeof(*data));
	if(!data){
		debug(2,"Shuffle struct malloc failed.");
		return NULL;
	}
	data->items=items;
	data->prime=*nextprime;
	data->count=data->searches=0;

	unsigned int a = (random()%13)+1;
	unsigned int b = (random()%7)+1;
	unsigned int c = (random()%5)+1;
	data->current = (random()%items);
	data->skip = (a*items*items)+(b*items)+c;
	data->skip &= ~0xC0000000;
	if(data->skip%data->prime==0)data->skip++;

	return data;
}

static int shuffle_cb(void *arg, int col_count, char **row, char **titles){
	unsigned int order;
	struct prime_rand_data *data=(struct prime_rand_data*)arg;
	if((order=prime_rand_next(data))==-1){
		debug(2,"prime_rand_next returned -1");
		return 1;
	}
	sprintf(data->query,"DELETE FROM TempPlaylistSong WHERE PlaylistSongID=%s",*(row+1));
	sqlite3_exec(conn,data->query,NULL,NULL,NULL);
	sprintf(data->query,"INSERT INTO TempPlaylistSong(SongID,\"Order\") VALUES(%s,%d)",*row,order+1);
	sqlite3_exec(conn,data->query,NULL,NULL,NULL);
	data->count++;
	if(data->count>DB_BATCH_SIZE){
		data->count=0;
		sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
		sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	}
	return 0;
}

void shuffle(int list){
	char query[250],cb_query[250];
	unsigned int items=0;

	if(list){
		sprintf(query,"SELECT COUNT(SongID) FROM PlaylistSong WHERE PlaylistID=%d",list);
		sqlite3_exec(conn,query,uint_return_cb,&items,NULL);
		sprintf(query,"SELECT SongID,PlaylistSongID FROM PlaylistSong WHERE PlaylistID=%d",list);
		createTempPlaylistSong();
	}
	else{
		sprintf(query,"SELECT COUNT(SongID) FROM TempPlaylistSong");
		sqlite3_exec(conn,query,uint_return_cb,&items,NULL);
		sprintf(query,"SELECT SongID,PlaylistSongID FROM TempPlaylistSong");
	}
	
	struct prime_rand_data *data=prime_rand_init(items);
	if(!data)
		return;
	data->query=cb_query;

	sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	sqlite3_exec(conn,query,shuffle_cb,data,NULL);
	sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
	free(data);
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
	sqlite3_exec(conn,data->query,NULL,NULL,NULL);
	sprintf(data->query,"UPDATE TempPlaylistSong SET \"Order\"=%d  WHERE PlaylistSongID=%s",id,*(row+1));
	sqlite3_exec(conn,data->query,NULL,NULL,NULL);

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
	sqlite3_exec(conn,data->query,NULL,NULL,NULL);
	sprintf(data->query,"UPDATE TempPlaylistSong SET \"Order\"=%d  WHERE PlaylistSongID=%s",neworder,*(row+1));
	sqlite3_exec(conn,data->query,NULL,NULL,NULL);

	data->count++;
	if(data->count>DB_BATCH_SIZE){
		data->count=0;
		sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
		sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
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
	sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	sqlite3_exec(conn,query,zs_slide_cb,&data,NULL);
	sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);

	// Play Count
	data.slidemod=data.slide*-2;
	data.count=0;
	sprintf(query,"SELECT Song.SongID,PlaylistSongID,\"Order\" FROM TempPlaylistSong NATURAL JOIN Song WHERE PlayCount>0 ORDER BY SkipCount DESC LIMIT %d",mod_count);
	sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	sqlite3_exec(conn,query,zs_slide_cb,&data,NULL);
	sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);

	// Rating
	data.slidemod=data.slide*-1;
	data.count=0;
	sprintf(query,"SELECT Song.SongID,PlaylistSongID,\"Order\",Rating FROM TempPlaylistSong NATURAL JOIN Song WHERE Rating>3 ORDER BY SkipCount DESC LIMIT %d",mod_count);
	sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
	sqlite3_exec(conn,query,zs_slide_cb,&data,NULL);
	sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);

	// Last Play
	data.count=data.increment;
	sprintf(query,"SELECT COUNT(PlaylistSongID) FROM TempPlaylistSong NATURAL JOIN Song WHERE LastPlay=0");
	sqlite3_exec(conn,query,uint_return_cb,&x,NULL);
	if(x>ten_percent){
		debug(1,"ZSHUFFLE | Too many unplayed songs; skipping LastPlay modifier.");
		return;
	}
	for(x=0;x<mod_count;x+=group){
		// What have I done...
		sprintf(query,"SELECT SongID,PlaylistSongID,\"Order\" FROM TempPlaylistSong WHERE SongID IN (SELECT SongID FROM Song NATURAL JOIN TempPlaylistSong ORDER BY LastPlay ASC LIMIT %d,%d) ORDER BY \"Order\" ASC",x,group);
		sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
		sqlite3_exec(conn,query,zs_skip_cb,&data,NULL);
		sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
	}
}


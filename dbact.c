/*
 *  Copyright (C) 2009-2013  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "dbact.h"
#include "defs.h"
#include "util.h"


int harp_sqlite3_exec(sqlite3 *connection, const char *sql, int (*callback)(void*,int,char**,char**), void *arg, char **errmsg){
	return sqlite3_exec(connection,sql,callback,arg,errmsg);
}

int db_exec_file(char *file){
	FILE *ffd;
	int x=0,querysize=200,ret=0;
	char *query=malloc(sizeof(char)*querysize);
	char *err;

	if(!query){
		debug(2,"Cannot malloc query");
		return 1;
	}

	if((ffd=fopen(file,"r"))){
		while(fgets((query+x),querysize-x,ffd)){
			//fprintf(stderr,"Next batch\n%x %x ",*ptr, query[x]);
			for(;x<querysize-1 && query[x]!='\n' && query[x];x++);
			if(x<querysize-1){
				query[x]=' ';
				query[++x]=0;
			}
			if(!sqlite3_complete(query)){
				if(x>=querysize-100){
					querysize+=100;
					if(querysize<2000)
						query=realloc(query,querysize);
					else{
						debug(2,"Limit reached");
						ret=1;break;
					}
					if(!query){
						debug(2,"Not enough memory");
						ret=1;break;
					}
				}
				continue;
			}

			if(harp_sqlite3_exec(conn,query,NULL,NULL,&err)!=SQLITE_OK){
				if(err){
					fprintf(stderr,"SQL Error: %s\n",err);
					sqlite3_free(err);
				}
				debug(2,"harp_sqlite3_exec error.");
				ret=1;break;
			}
			x=0;
		}
		fclose(ffd);
	}
	else{
		fprintf(stderr,"Cannot open file\n");
		ret=1;
	}
	free(query);
	return ret;
}

unsigned int dbInit(){
	char temp[255];
	//strcpy(temp,DB);
	sprintf(temp,"%s%s",DB_PATH,DB);
	expand(temp);
	if(sqlite3_open_v2(temp,&conn,SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_READWRITE,NULL)!=SQLITE_OK){
		printf("Database not found. Attempting to create\n");
		// Try creating
		strcpy(temp,DB_PATH);
		expand(temp);
		mkdir(temp,0700);
		sprintf(temp,"%s%s",DB_PATH,DB);
		expand(temp);
		if(sqlite3_open(temp,&conn)!=SQLITE_OK){
			printf("Database connection error: %s\n",sqlite3_errmsg(conn));
			return 0;
		}
		sprintf(temp,"%s/harp/create.sql",SHARE_PATH);
		if(db_exec_file(temp)){
			printf("Error creating database from file:\n\t%s\n",temp);
			sqlite3_close(conn);
			return 0;
		}
		fprintf(stderr,"Please add any installed plugins to the database with harp -a\n");
	}
	return 1;
}

void dbiInit(struct dbitem *dbi){
	dbi->result=NULL;
	dbi->row=NULL;
	dbi->err=NULL;
}

void dbiClean(struct dbitem *dbi){
	if(dbi->row!=NULL){
		free(dbi->row);
		dbi->row=NULL;
	}
	if(dbi->result!=NULL){
		sqlite3_free_table(dbi->result);
		dbi->result=NULL;
	}
}

int fetch_row(struct dbitem *dbi){
	if(dbi->row_count==0)return 0;

	int x;
	for(x=0;x<dbi->column_count;x++)
		dbi->row[x]=dbi->result[dbi->current_row+x];

	x=dbi->current_row<(dbi->row_count+1)*dbi->column_count?1:0;
	dbi->current_row+=dbi->column_count;
	return x;
}

int fetch_row_at(struct dbitem *dbi,int index){
	int x;
	if(index>=dbi->row_count || index<0)return 0;
	for(x=0;x<dbi->column_count;x++)
		dbi->row[x]=dbi->result[index+x];
	return 1;
}

char ** fetch_column_at(struct dbitem *dbi, int index){
	int x;
	char **col;
	if(!(col=malloc(sizeof(char*)*dbi->row_count)))return NULL;
	index+=dbi->column_count;
	for(x=0;x<dbi->row_count;x++)
		col[x]=dbi->result[index+(x*dbi->column_count)];
	return col;
}

int doQuery(const char *querystr,struct dbitem *dbi){
	dbiClean(dbi);
	sqlite3_get_table(conn,querystr,&dbi->result,&dbi->row_count,&dbi->column_count,&dbi->err);
	if(dbi->err!=NULL){
		fprintf(stderr,"Database query error: %s\n",dbi->err);
		sqlite3_free(dbi->err);
		dbi->err=NULL;
		return -1;
	}
	if(!dbi->column_count)
		return sqlite3_changes(conn);

	dbi->current_row=0;
	if(!(dbi->row=malloc(sizeof(char*)*dbi->column_count))){
		debug(2,"Malloc failed (dbi->row).");
		return -1;
	}
	fetch_row(dbi);
	return dbi->row_count;
}

int uint_return_cb(void *arg, int col_count, char **row, char **titles){
	*(unsigned int*)arg=(unsigned int)strtol(*row,NULL,10);
	return 0;
}

int str_return_cb(void *arg, int col_count, char **row, char **titles){
	arg=strdup(*row);
	return 0;
}

static int titlequery_titles_cb(void *data, int col_count, char **row, char **titles){
	struct titlequery_data *arg=(struct titlequery_data*) data;
	int x,templen,len=arg->maxwidth+3;
	col_count--;
	for(x=0;x<col_count;x++){
		if(!arg->exception[x]){
			len=printf("[%.*s]",arg->maxwidth,titles[x]);
			printf("%*c",(arg->maxwidth+3)-len,' ');
		}
		else{
			templen=printf("[%s]",titles[x]);
			if(templen>arg->exlen[x])arg->exlen[x]=templen;
			printf("%*c",(arg->exlen[x]-templen)+1,' ');
		}
	}
	if(!arg->exception[x]){ // Ignore end spacing on final field
		printf("[%.*s]",arg->maxwidth,titles[x]);
	}
	else
		printf("[%s] ",titles[x]);
	printf("\n");
	return 1;
}

static int titlequery_columns_cb(void *data, int col_count, char **row, char **titles){
	struct titlequery_data *arg=(struct titlequery_data*) data;
	int x,templen;
	char *ptr;
	col_count--;
	for(x=0;x<col_count;x++){
		if(arg->exception[x]){
			for(ptr=row[x];*ptr;++ptr);
			templen=(ptr-row[x])+2;
			if(templen>arg->exlen[x])arg->exlen[x]=templen;
		}
	}
	return 0;
}

static int titlequery_cb(void *data, int col_count, char **row, char **titles){
	struct titlequery_data *arg=(struct titlequery_data*) data;
	int x,templen,len=arg->maxwidth+3;
	col_count--;
	for(x=0;x<col_count;x++){
		if(!arg->exception[x]){
			len=printf("[%.*s]",arg->maxwidth,row[x]);
			printf("%*c",(arg->maxwidth+3)-len,' ');
		}
		else{
			templen=printf("[%s]",row[x]);
			printf("%*c",(arg->exlen[x]-templen)+1,' ');
		}
	}
	if(!arg->exception[x]){ // Ignore end spacing on final field
		printf("[%.*s]",arg->maxwidth,row[x]);
	}
	else
		printf("[%s] ",row[x]);
	printf("\n");
	arg->count++;
	return 0;
}

int doTitleQuery(const char *querystr,int *exception, int maxwidth){
	struct titlequery_data tqd;

	int defexception[10];
	if(!exception){
		int x;
		for(x=1;x<10;x++)defexception[x]=listconf.exception;
		exception=defexception;
	}

	tqd.count=0;
	tqd.exception=exception;
	tqd.maxwidth=maxwidth;
	tqd.exlen=calloc(10,sizeof(int));

	harp_sqlite3_exec(conn,querystr,titlequery_columns_cb,&tqd,NULL);
	harp_sqlite3_exec(conn,querystr,titlequery_titles_cb,&tqd,NULL);
	harp_sqlite3_exec(conn,querystr,titlequery_cb,&tqd,NULL);

	free(tqd.exlen);
	return tqd.count;
}

int batch_tempplaylistsong_insert_cb(void *arg, int col_count, char **row, char **titles){
	struct insert_tps_arg *data=(struct insert_tps_arg*)arg;

	sprintf(data->query,"INSERT INTO TempPlaylistSong(SongID,\"Order\") VALUES(%s,%d)",*row,data->order);
	harp_sqlite3_exec(conn,data->query,NULL,NULL,NULL);

	data->order++;
	return 0;
}

void createTempPlaylistSong(){
	harp_sqlite3_exec(conn,"CREATE TEMP TABLE IF NOT EXISTS TempPlaylistSong( PlaylistSongID integer primary key, SongID integer not null, \"Order\" integer NOT NULL)",NULL,NULL,NULL);
}

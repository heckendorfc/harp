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

#include "util.h"
#include "defs.h"
#include "dbact.h"

int experr(const char *epath, int eerrno){
	fprintf(stderr,"error: %d on %s\n",eerrno,epath);
	return 0;
}

int isURL(const char *in){
	if(strncmp("http://",in,7)==0)
		return 1;
	return 0;
}

char *expand(char *in){
	char tmp[250];
	char *in_ptr=in-1;
	char *tmp_ptr=tmp;
	int url=0,x,y,z=0;

	if(isURL(in))url=1; /* Clean \n but do no globbing */

	for(x=0;x<249 && in[x];x++);
	if(x>0 && in[x-1]=='\n')
		in[x-1]=0;
	if(x>1 && in[x-2]=='\r')
		in[x-2]=0;
	in[x]=0;
	y=x;

	if(url)return in;

	if(in[0]=='~'){
		strcpy(tmp,getenv("HOME"));
		strcat(tmp,"/");
		strcat(tmp,in+2);
		strcpy(in,tmp);
	}
	else if(in[0]!='/'){
		strcpy(tmp,getenv("PWD"));
		strcat(tmp,"/");
		if(in[0]=='.' && in[1]=='/')
			strcat(tmp,in+2);
		else
			strcat(tmp,in);
		strcpy(in,tmp);
	}
	while(*(++in_ptr)){
		if(*in_ptr=='[' || *in_ptr=='\'' || *in_ptr=='\"')
			*(tmp_ptr++)='\\';
		*(tmp_ptr++)=*in_ptr;
	}
	*tmp_ptr=0;


	glob_t pglob;
	glob(tmp,GLOB_TILDE,&experr,&pglob);
	if(pglob.gl_pathc>0){
		memmove(tmp,pglob.gl_pathv[0],100);
		debug(1,"Glob OK!");
	}
	else
		debug(1,"No files match");
	globfree(&pglob);
	return in;
}

int fileFormat(struct pluginitem *list, const char *arg){
	FILE *ffd=NULL;
	char query[200];
	struct dbitem dbi;
	struct pluginitem *ret=list;
	int type=0;
	
	// Find by magic numbers

	while(!type && ret){
		if((ffd=ret->modopen(arg,"rb"))!=NULL){
			if(ret->moddata(ffd)){
				sprintf(query,"SELECT TypeID FROM PluginType WHERE PluginID=%d",ret->id);
				sqlite3_exec(conn,query,uint_return_cb,&type,NULL);
			}
			ret->modclose(ffd);
		}
		ret=ret->next;
	}
	if(ffd==NULL)
		type=0;
	if(type){
		return type;
	}

	// Find by extension
	dbiInit(&dbi);
	doQuery("SELECT TypeID,Extension FROM FileExtension",&dbi);
	int size=0,x=0;
	for(x=0;arg[x];x++);size=x;
	for(;arg[x-1]!='.' && x>0;x--);
	while(fetch_row(&dbi)){
		if(strcmp(arg+x,dbi.row[1])==0){
			type=(int)strtol(dbi.row[0],NULL,10);
			break;
		}
	}
	dbiClean(&dbi);
	return type;
}

int getFileTypeByName(const char *name){
	char query[200];
	int type=0;

	if(!name)return 0;

	sprintf(query,"SELECT TypeID FROM FileType WHERE Name='%s' LIMIT 1",name);
	sqlite3_exec(conn,query,uint_return_cb,&type,NULL);

	return type;
}

int findPluginIDByType(int type){
	static int last_type=0;
	static int index=0;
	unsigned int id=0;
	char query[150];

	if(type==0 || (type!=last_type && last_type!=0)){
		last_type=index=0;
		if(type==0)return 0;
	}
	last_type=type;

	sprintf(query,"SELECT PluginID FROM PluginType WHERE TypeID=%d LIMIT %d,1",type,index);

	sqlite3_exec(conn,query,uint_return_cb,&id,NULL);
	index++;

	return id;
}

struct pluginitem *findPluginByID(struct pluginitem *list, int id){
	while(list && list->id!=id)
		list=list->next;
	return list;
}

int getPluginModule(void **module, char *lib){
	char library[250];

	sprintf(library,"%s/%s",LIB_PATH,lib);
	debug(2,library);
	dlerror();

	*module=dlopen(library,RTLD_LAZY);
	if(!*module){
		fprintf(stderr,"Can't open plugin.\n\t%s\n",dlerror());
		return 2;
	}
	else{
		dlerror();
	}

	return 1;
}

int getPlugin(struct dbitem *dbi, const int index, void **module){
	char library[250];

	if(!fetch_row(dbi)){
		if(dbi->current_row==dbi->column_count || dbi->row_count==0)
			fprintf(stderr,"No plugins found.\n\tSee README for information about installing plugins.\n");
		return 0;
	}

	return getPluginModule(module,dbi->row[index]);
}

struct pluginitem *closePlugin(struct pluginitem *head){
	struct pluginitem *ptr=head;

	if(!head)return NULL;

	ptr=ptr->next;
	if(head->module)
		dlclose(head->module);
	free(head);

	return ptr;
}

void closePluginList(struct pluginitem *head){
	struct pluginitem *ptr=head;

	do{
		ptr=closePlugin(ptr);
	}while(ptr);
}

static int regPluginFunctions(struct pluginitem *node){
	if(!node->module)return 1;

	if(!(node->modopen=dlsym(node->module,"plugin_open")) ||
		!(node->moddata=dlsym(node->module,"filetype_by_data")) ||
		!(node->modmeta=dlsym(node->module,"plugin_meta")) ||
		!(node->modplay=dlsym(node->module,"plugin_run")) ||
		!(node->modseek=dlsym(node->module,"plugin_seek")) ||
		!(node->modclose=dlsym(node->module,"plugin_close")))
		return 1;

	return 0;
}

static int open_plugin_cb(void *arg, int col_count, char **row, char **titles){
	struct pluginitem **node = (struct pluginitem**)arg;
	struct pluginitem *next;

	if(!(next=malloc(sizeof(struct pluginitem)))){
		debug(2,"Malloc failed (open_plugin_cb).");
		return 1;
	}
	if(getPluginModule(&(next->module),row[1])!=1){
		free(next);
			return 1;
	}
	else{
		if(regPluginFunctions(next)){
			free(next);
			return 1;
		}
		else{
			(*node)->next=next;
			*node=next;
			next->next=NULL;
			next->id=strtol(row[0],NULL,10);
			next->contenttype=strdup(row[2]);
		}
	}

	return 0;
}

struct pluginitem *openPlugins(){
	int count;
	struct pluginitem prehead,*ptr;

	ptr=&prehead;
	// Add order by active?
	if(sqlite3_exec(conn,"SELECT Plugin.PluginID,Plugin.Library,FileType.ContentType FROM Plugin NATURAL JOIN PluginType NATURAL JOIN FileType",open_plugin_cb,&ptr,NULL)!=SQLITE_OK){
		closePluginList(prehead.next);
		fprintf(stderr,"Error opening plugins.\n");
		return NULL;
	}

	return prehead.next; /* List starts at this next */
}

char *getFilename(const char *path){
	char *filestart=(char *)path;
	while(*path){
		if(*(path++)=='/')
			filestart=(char *)path;
	}
	return filestart;
}

int strToID(const char *argv){ // TODO: add type param
	char query[201];
	int id=0,found=0;
	struct dbitem dbi;

	if(!arglist[ATYPE].subarg)return -1;
	dbiInit(&dbi);

	switch(arglist[ATYPE].subarg[0]){
		case 's':sprintf(query,"SELECT SongID,Title FROM Song WHERE Title LIKE '%%%s%%'",argv);break;
		case 'p':sprintf(query,"SELECT PlaylistID,Title FROM Playlist WHERE Title LIKE '%%%s%%'",argv);break;
		case 'r':sprintf(query,"SELECT ArtistID,Name FROM Artist WHERE Name LIKE '%%%s%%'",argv);break;
		case 'a':sprintf(query,"SELECT AlbumID,Title FROM Album WHERE Title LIKE '%%%s%%'",argv);break;
		case 'g':sprintf(query,"SELECT CategoryID,Name FROM Category WHERE Name LIKE '%%%s%%'",argv);break;
		default:return -1;
	}

	if(doQuery(query,&dbi)==1 && fetch_row(&dbi)){
		id=(int)strtol(dbi.row[0],NULL,10);
	}
	else if(dbi.row_count>1){
		printf("Total matches for string '%s': %d\n",argv,dbi.row_count);
		while(fetch_row(&dbi)){
			printf("%s\t%s\n",dbi.row[0],dbi.row[1]);
			if(!found){
				if(!strcmp(argv,dbi.row[1])){ // Perfect matches take priority
					id=(int)strtol(dbi.row[0],NULL,10);
					found=id?1:0;
				}
				else if(!strcasecmp(argv,dbi.row[1])){  // Case-insensitive matches can be used
					id=(int)strtol(dbi.row[0],NULL,10); // Continue to look for perfect matches
				}
			}
		}
		if(id)
			printf("Using best match: %d\n",id);
	}

	dbiClean(&dbi);
	return id;
}

int verifyID(const int id){
	char query[100];
	struct dbitem dbi;
	dbiInit(&dbi);
	switch(arglist[ATYPE].subarg[0]){
		case 's':sprintf(query,"SELECT SongID FROM Song WHERE SongID=%d",id);break;
		case 'p':sprintf(query,"SELECT PlaylistID FROM Playlist WHERE PlaylistID=%d",id);break;
		case 'r':sprintf(query,"SELECT ArtistID FROM Artist WHERE ArtistID=%d",id);break;
		case 'a':sprintf(query,"SELECT AlbumID FROM Album WHERE AlbumID=%d",id);break;
		case 'g':sprintf(query,"SELECT CategoryID FROM Category WHERE CategoryID=%d",id);break;
		default:return 0;
	}
	if(doQuery(query,&dbi)==1){
		dbiClean(&dbi);
		return 1;
	}
	dbiClean(&dbi);
	return 0;
}

int getID(const char *arg){
	int id;
	char *endptr;
	if(arg==NULL){
		printf("Required argument not provided\n");
		return -1;
	}
	id=(int)strtol(arg,&endptr,10);
	if(*endptr!=0){
		if((id=strToID(arg))<1 ){
			debug(1,"No ID found from given name.");
			return -1;
		}
	}
	if(!verifyID(id)){
		debug(1,"No ID found.");
		return -1;
	}
	return id;
}

int *getMulti(char *arg, int *length){
	const char del[]=",";
	char *token;
	int *list,*ptr;

	*length=1;
	token=arg;
	while(*token){
		if(*(token++)==',')
			(*length)++;
	}
	if(!(ptr=list=malloc(sizeof(int)*(*length)))){
		debug(2,"Malloc failed (list).");
		return NULL;
	}

	while((token=strsep(&arg,del))){
		if(!((*ptr=(int)strtol(token,NULL,10))>0 && verifyID(*ptr))){
			if(!(*ptr=strToID(token))){
				continue;
			}
		}
		ptr++;
	}
	if(ptr==list)*list=-1;

	return list;
}

void cleanTempSelect(const int tempid){
	char query[100];
	sprintf(query,"DELETE FROM TempSelect WHERE TempID=%d",tempid);
	sqlite3_exec(conn,query,NULL,NULL,NULL);
}

static void createTempSelect(){
	sqlite3_exec(conn,"CREATE TEMP TABLE IF NOT EXISTS TempSelect(TempSelectID integer not null primary key, TempID integer not null, SelectID integer not null)",NULL,NULL,NULL);
}

static int getNextTempSelectID(){
	int tempid=0;
	/* Replace with MAX(TempID)? */
	if(sqlite3_exec(conn,"SELECT TempID FROM TempSelect ORDER BY TempID DESC LIMIT 1",uint_return_cb,&tempid,NULL)==SQLITE_DONE)
		return 1;
	return ++tempid;
}

int insertTempSelect(const int *ids, const int idlen){
	unsigned int x,currentlimit,tempid;
	struct dbitem dbi;
	dbiInit(&dbi);
	char query[150];

	createTempSelect();

	tempid=getNextTempSelectID();

	x=currentlimit=0;
	while(x<idlen){
		if((currentlimit+=DB_BATCH_SIZE)>idlen)currentlimit=idlen;
		sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
		for(;x<currentlimit;x++){
			sprintf(query,"INSERT INTO TempSelect(TempID,SelectID) VALUES(%d,%d)",tempid,ids[x]);
			sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
	}

	return tempid;
}

int insertTempSelectQuery(const char *query){
	const char *ins_q="INSERT INTO TempSelect(TempID,SelectID) ";
	unsigned int x,currentlimit,tempid;
	char *temp_q,*ptr;

	if(!(temp_q=malloc(strlen(ins_q)+strlen(query)+1))){
		debug(2,"Malloc failed (tempquery)");
		exit(1);
	}

	createTempSelect();

	tempid=getNextTempSelectID();
	sprintf(temp_q,query,tempid);
	ptr=strdup(temp_q);
	sprintf(temp_q,"%s%s",ins_q,ptr);
	debug(2,temp_q);

	sqlite3_exec(conn,temp_q,NULL,NULL,NULL);
	
	free(temp_q);
	free(ptr);
	return tempid;
}

void miClean(struct musicInfo *mi){
	memset(mi->title,0,MI_TITLE_SIZE);
	memset(mi->album,0,MI_ALBUM_SIZE);
	memset(mi->artist,0,MI_ARTIST_SIZE);
	memset(mi->year,0,MI_YEAR_SIZE);
	memset(mi->track,0,MI_TRACK_SIZE);
	mi->length=insertconf.length;
}

void miFree(struct musicInfo *mi){
	if(mi->title!=NULL)
		free(mi->title);
	if(mi->album!=NULL)
		free(mi->album);
	if(mi->artist!=NULL)
		free(mi->artist);
	if(mi->year!=NULL)
		free(mi->year);
	if(mi->track!=NULL)
		free(mi->track);
}

void db_clean(char *str, const char *data, const size_t size){
	int x,z;
	for(x=0;*(data+x)==' ' && x<size;x++); // Strip starting spaces
	while(*data>31 && *data<127 && x++<size){ // Strip multi space
		if(*data==' ' && *(data+1)==' '){
			data++;
			continue;
		}
		*(str++)=*(data++);
	}
	if(x>0 && *(str-1)==' ')str--; // Remove trailing space
	*str=0;
}

void db_safe(char *str, const char *data, const size_t size){
	int x=0;
	while(*data && x++<size){ // Escape single quotes
		if(*data=='\''){
			*(str++)='\'';
		}
		*(str++)=*(data++);
	}
	*str=0;
}


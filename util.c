/*
 *  Copyright (C) 2009-2014  Christian Heckendorf <heckendorfc@gmail.com>
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
	return strncmp("http://",in,7)==0;
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
				harp_sqlite3_exec(conn,query,uint_return_cb,&type,NULL);
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
	for(x=0;arg[x];x++);
	//size=x;
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
	harp_sqlite3_exec(conn,query,uint_return_cb,&type,NULL);

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

	harp_sqlite3_exec(conn,query,uint_return_cb,&id,NULL);
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

	prehead.next=NULL;
	ptr=&prehead;
	// Add order by active?
	if(harp_sqlite3_exec(conn,"SELECT Plugin.PluginID,Plugin.Library,FileType.ContentType FROM Plugin NATURAL JOIN PluginType NATURAL JOIN FileType",open_plugin_cb,&ptr,NULL)!=SQLITE_OK){
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

static void printIDMatchRow(struct dbitem *dbi){
	int x;
	printf("%s\t%s",dbi->row[0],dbi->row[1]);
	if(dbi->column_count>2){
		printf("\t(%s",dbi->row[2]);
		for(x=3;x<dbi->column_count;x++)
			printf(": %s",dbi->row[x]);
		printf(")");
	}
	printf("\n");
}

int getBestIDMatch(struct dbitem *dbi, const char *argv){
	int id=0,found=0;
	if(!dbi)return 0;

	printf("Total matches for string '%s': %d\n",argv,dbi->row_count);
	while(fetch_row(dbi)){
		printIDMatchRow(dbi);
		if(!strcmp(argv,dbi->row[1])){ // Perfect matches take priority
			id=(int)strtol(dbi->row[0],NULL,10);
			if(id)found++;
		}
		else if(!strcasecmp(argv,dbi->row[1])){  // Case-insensitive matches can be used
			id=(int)strtol(dbi->row[0],NULL,10); // Continue to look for perfect matches
		}
	}
	if(id && found==1)
		printf("Using best match: %d\n",id);
	else{
		char choice[100];
		printf("Please choose an ID: ");
		if(!fgets(choice,sizeof(choice),stdin))
			id=0;
		else
			id=(int)strtol(choice,NULL,10);
	}
	return id;
}

int strToID(const char *argv){ // TODO: add type param
	char query[351];
	char clean_str[200];
	int id=0;
	struct dbitem dbi;

	db_safe(clean_str,argv,200);

	if(!arglist[ATYPE].subarg)return -1;
	dbiInit(&dbi);

	switch(arglist[ATYPE].subarg[0]){
		case 's':sprintf(query,"SELECT SongID,SongTitle,ArtistName,AlbumTitle FROM SongPubInfo WHERE SongTitle LIKE '%%%s%%'",clean_str);break;
		case 'p':sprintf(query,"SELECT PlaylistID,Title FROM Playlist WHERE Title LIKE '%%%s%%'",clean_str);break;
		case 'r':sprintf(query,"SELECT ArtistID,Name FROM Artist WHERE Name LIKE '%%%s%%'",clean_str);break;
		case 'a':sprintf(query,"SELECT AlbumID,Title,Artist.Name FROM Album NATURAL JOIN AlbumArtist NATURAL JOIN Artist WHERE Title LIKE '%%%s%%'",clean_str);break;
		case 'g':sprintf(query,"SELECT CategoryID,Name FROM Category WHERE Name LIKE '%%%s%%'",clean_str);break;
		case 't':sprintf(query,"SELECT TagID,Value FROM Tag WHERE Value LIKE '%%%s%%'",clean_str);break;
		default:return -1;
	}

	if(doQuery(query,&dbi)==1 && fetch_row(&dbi)){
		id=(int)strtol(dbi.row[0],NULL,10);
	}
	else if(dbi.row_count>1)
		id=getBestIDMatch(&dbi,argv);

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
		case 't':sprintf(query,"SELECT TagID FROM Tag WHERE TagID=%d",id);break;
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

int getGroupSongIDs(char *args, const int arglen, struct IDList *id_struct){
	int y,x=0;
	int *song_ids=malloc(sizeof(int));
	int *group_ids=malloc(sizeof(int));
	int song_idlen;
	int group_idlen;
	char query[200],query_tail[100],*ptr;
	char temp=0;
	struct dbitem dbi;
	dbiInit(&dbi);
	query_tail[0]=0;

	/* Default should be hit. Avoid selecting another option. */
	if(args[x+1]!=' '){
		temp=args[x];
		args[x]=' ';
	}

	switch(args[x]){
		case 'a':
			arglist[ATYPE].subarg[0]='a';
			sprintf(query,"SELECT SongID FROM Song INNER JOIN Album ON Album.AlbumID=Song.AlbumID WHERE Album.AlbumID=");
			sprintf(query_tail,"ORDER BY Track");
			ptr=&query[91];
			break;
		case 'r':
			arglist[ATYPE].subarg[0]='r';
			sprintf(query,"SELECT SongID FROM Song NATURAL JOIN AlbumArtist WHERE ArtistID=");
			sprintf(query_tail,"ORDER BY Song.AlbumID,Track");
			ptr=&query[64];
			break;
		case 't':
			arglist[ATYPE].subarg[0]='t';
			sprintf(query,"SELECT Song.SongID FROM Song NATURAL JOIN SongTag WHERE TagID=");
			sprintf(query_tail,"ORDER BY Song.AlbumID,Track");
			ptr=&query[62];
			break;
		case 'g':
			arglist[ATYPE].subarg[0]='g';
			sprintf(query,"SELECT SongID FROM Song NATURAL JOIN SongCategory WHERE CategoryID=");
			sprintf(query_tail,"ORDER BY Song.AlbumID,Track");
			ptr=&query[67];
			break;
		case 's':
			x++;
		default:
			if(temp)args[x]=temp;
			// List of SongIDs.
			for(;x<arglen && args[x] && args[x]==' ';x++);
			arglist[ATYPE].subarg[0]='s';
			if(!args[x] || !(song_ids=getMulti(&args[x],&song_idlen)) || song_ids[0]<1)
				return HARP_RET_ERR;
			id_struct->tempselectid=insertTempSelect(song_ids,song_idlen);
			id_struct->songid=song_ids;
			id_struct->length=song_idlen;
			return HARP_RET_OK;
	}

	debug(3,query);
	// Get SongIDs from album, artist, or genre
	for(++x;x<arglen && args[x] && args[x]==' ';x++);
	if(!(group_ids=getMulti(&args[x],&group_idlen)))return HARP_RET_ERR;
	if(group_ids[0]<1){
		fprintf(stderr,"No results found.\n");
		free(group_ids);
		return HARP_RET_ERR;
	}
	for(song_idlen=x=0;x<group_idlen;x++){
		sprintf(ptr,"%d %s",group_ids[x],query_tail);
		if(doQuery(query,&dbi)<1)continue;
		y=song_idlen;
		song_idlen+=dbi.row_count;
		if(!(song_ids=realloc(song_ids,sizeof(int)*(song_idlen)))){
			debug(2,"Realloc failed (song_ids).");
			dbiClean(&dbi);
			free(group_ids);
			return HARP_RET_ERR;
		}
		for(;fetch_row(&dbi);y++)
			song_ids[y]=(int)strtol(dbi.row[0],NULL,10);
	}
	dbiClean(&dbi);
	free(group_ids); // Done with groups. We have the SongIDs.

	if(!song_idlen){
		fprintf(stderr,"No results found.\n");
		free(song_ids);
		return HARP_RET_ERR;
	}
	id_struct->tempselectid=insertTempSelect(song_ids,song_idlen);
	id_struct->songid=song_ids;
	id_struct->length=song_idlen;

	return HARP_RET_OK;
}

void cleanTempSelect(const int tempid){
	char query[100];
	sprintf(query,"DELETE FROM TempSelect WHERE TempID=%d",tempid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
}

static void createTempSelect(){
	harp_sqlite3_exec(conn,"CREATE TEMP TABLE IF NOT EXISTS TempSelect(TempSelectID integer not null primary key, TempID integer not null, SelectID integer not null)",NULL,NULL,NULL);
}

static int getNextTempSelectID(){
	int tempid=0;
	/* Replace with MAX(TempID)? */
	if(harp_sqlite3_exec(conn,"SELECT TempID FROM TempSelect ORDER BY TempID DESC LIMIT 1",uint_return_cb,&tempid,NULL)==SQLITE_DONE)
		return 1;
	return ++tempid;
}

int mergeTempSelect(int ida, int idb){
	char query[150];

	if(ida==idb)
		return ida;

	sprintf(query,"UPDATE TempSelect SET TempID=%d where TempID=%d",ida,idb);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return ida;
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
		harp_sqlite3_exec(conn,"BEGIN",NULL,NULL,NULL);
		for(;x<currentlimit;x++){
			sprintf(query,"INSERT INTO TempSelect(TempID,SelectID) VALUES(%d,%d)",tempid,ids[x]);
			harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
		}
		harp_sqlite3_exec(conn,"COMMIT",NULL,NULL,NULL);
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

	harp_sqlite3_exec(conn,temp_q,NULL,NULL,NULL);

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


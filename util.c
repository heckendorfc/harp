/*
 *  Copyright (C) 2009  Christian Heckendorf <heckendorfc@gmail.com>
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


int experr(const char *epath, int eerrno){
	fprintf(stderr,"error: %d on %s\n",eerrno,epath);
	return 0;
}

char *expand(char *in){
	char tmp[250];
	char *in_ptr=in-1;
	char *tmp_ptr=tmp;
	int x,y,z=0;
	for(x=0;x<249 && in[x];x++);
	if(in[x-1]=='\n')
		in[x-1]=0;
	in[x]=0;
	y=x;
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

typedef int (*filetype_by_data)(FILE *ffd);

int fileFormat(const char *arg){
	FILE *ffd;
	char library[200];
	struct dbitem dbi;
	int ret=0;

	ffd=fopen(arg,"rb");
	if(ffd!=NULL){
		// Check if it is a file
		struct stat st;
		if(!stat(arg,&st) && !S_ISREG(st.st_mode)){
			fprintf(stderr,"\"%s\" is not a regular file.\n",arg);
			fclose(ffd);
			return -1;
		}

		// Find by magic numbers
		dbiInit(&dbi);
		doQuery("SELECT TypeID, Library FROM FilePlugin",&dbi);

		void *module;
		filetype_by_data data;
		while((ret=getPlugin(&dbi,1,&module))){
			if(ret>1)continue;
			data=dlsym(module,"filetype_by_data");
			if(!module){
				debug(2,"\nPlugin does not contain filetype_by_data().\n");
				//ret=-1;
			}
			else if(data(ffd)){
				ret=(int)strtol(dbi.row[0],NULL,10);
				dlclose(module);
				break;
			}
			dlclose(module);
		}
		fclose(ffd);
		if(ret){
			dbiClean(&dbi);
			return ret;
		}

		// Find by extension
		doQuery("SELECT TypeID,Extension FROM FileExtension",&dbi);
		int size=0,x=0;
		for(x=0;arg[x];x++);size=x;
		for(;arg[x-1]!='.' && x>0;x--);
		while(fetch_row(&dbi)){
			if(strcmp(arg+x,dbi.row[1])==0){
				ret=(int)strtol(dbi.row[0],NULL,10);
				break;
			}
		}
		dbiClean(&dbi);
		return ret;
	}
	else{
		fprintf(stderr,"Can't open file: %s\n",arg);
		return -1;
	}
}

int getPlugin(struct dbitem *dbi, const int index, void **module){
	char library[250];
	if(!fetch_row(dbi)){
		if(dbi->current_row==dbi->column_count || dbi->row_count==0)
			fprintf(stderr,"No plugins found.\n\tSee README for information about installing plugins.\n");
		return 0;
	}
	sprintf(library,"%s/%s",LIB_PATH,dbi->row[index]);
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

int insertTempSelect(const int *ids, const int idlen){
	unsigned int x,currentlimit,tempid;
	struct dbitem dbi;
	dbiInit(&dbi);
	char query[150];

	sqlite3_exec(conn,"CREATE TEMP TABLE IF NOT EXISTS TempSelect(TempSelectID integer not null primary key, TempID integer not null, SelectID integer not null)",NULL,NULL,NULL);

	// Replace with MAX(TempID)?
	sqlite3_exec(conn,"SELECT TempID FROM TempSelect ORDER BY TempID DESC LIMIT 1",uint_return_cb,&tempid,NULL);
	tempid++;

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

void miClean(struct musicInfo *mi){
	memset(mi->title,0,MI_TITLE_SIZE);
	memset(mi->album,0,MI_ALBUM_SIZE);
	memset(mi->artist,0,MI_ARTIST_SIZE);
	memset(mi->year,0,MI_YEAR_SIZE);
	memset(mi->track,0,MI_TRACK_SIZE);
	memset(mi->length,0,MI_LENGTH_SIZE);
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
	if(mi->length!=NULL)
		free(mi->length);
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


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


int experr(const char *epath, int eerrno){
	fprintf(stderr,"error: %d on %s\n",eerrno,epath);
	return 0;
}

char *expand(char *in){
	char tmp[250];
	int x,y,z=0;
	for(x=0;x<249 && in[x];x++);if(in[x-1]=='\n')in[x-1]=0;in[x]=0;
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
	for(x=0;in[x]!=0;x++){
		if(in[x]=='[' || in[x]=='\'' || in[x]=='\"'){
			tmp[z]='\\';
			z++;
		}
		tmp[z]=in[x];
		z++;
	}
	tmp[z]=0;
	//strcpy(in,tmp);


	glob_t pglob;
	glob(tmp,GLOB_TILDE,&experr,&pglob);
	if(pglob.gl_pathc>0){
		memmove(tmp,pglob.gl_pathv[0],100);

		debug("Ok!");
	}
	else
		debug("No files match");
	globfree(&pglob);
	return in;
}

	/*
	if(buf[0]==0x30 && buf[1]==0x26 && buf[2]==0xB2 && buf[3]==0x75){
		debug("wma(asf)\n");
		fclose(fd);
		return FASF;
	}
	*/

typedef int (*filetype_by_data)(FILE *ffd);

int fileFormat(char *arg){
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
				//debug(dlerror());
				debug("\nPlugin does not contain filetype_by_data().\n");
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
			if(strcmp(&arg[x],dbi.row[1])==0){
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

int getPlugin(struct dbitem *dbi, int index, void **module){
	char library[250];
	if(!fetch_row(dbi)){
		if(dbi->current_row==dbi->column_count || dbi->row_count==0)
			fprintf(stderr,"No plugins found.\n\tSee README for information about installing plugins.\n");
		return 0;
	}
	sprintf(library,"%s/%s",LIB_PATH,dbi->row[index]);
	debug(library);
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

char *getFilename(char *path){
	char *filestart=path;
	while(*path){
		if(*path=='/')
			filestart=path+1;
		path++;
	}
	return filestart;
}

int isNumeric(char *argv){
	while(*argv){
		if(!isdigit(*argv))
			return 0;
		argv++;
	}
	return 1;
}

int getID(char *arg){
	int id;
	char *endptr;
	if(arg==NULL){
		printf("Required argument not provided\n");
		return -1;
	}
	id=(int)strtol(arg,&endptr,10);
	if(*endptr!=0){
		if((id=strToID(arg))<1){
			debug("No ID found.");
			return -1;
		}
	}
	return id;
}

int strToID(char *argv){ // TODO: add type param
	char query[201];
	int id=0,found=0;
	struct dbitem dbi;
	dbiInit(&dbi);
	switch(arglist[ATYPE].subarg[0]){
		case 's':sprintf(query,"SELECT SongID,Title FROM Song WHERE Title LIKE '%%%s%%'",argv);break;
		case 'p':sprintf(query,"SELECT PlaylistID,Title FROM Playlist WHERE Title LIKE '%%%s%%'",argv);break;
		case 'r':sprintf(query,"SELECT ArtistID,Name FROM Artist WHERE Name LIKE '%%%s%%'",argv);break;
		case 'a':sprintf(query,"SELECT AlbumID,Title FROM Album WHERE Title LIKE '%%%s%%'",argv);break;
		default:return -1;
	}
	if(doQuery(query,&dbi)==1 && fetch_row(&dbi)){
		id=(int)strtol(dbi.row[0],NULL,10);
		dbiClean(&dbi);
		return id;
	}
	if(dbi.row_count>1){
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
		if(id){
			printf("Using best match: %d\n",id);
			dbiClean(&dbi);
			return id;
		}
	}
	dbiClean(&dbi);
	return 0;
}

int verifyID(int id){
	char query[100];
	struct dbitem dbi;
	dbiInit(&dbi);
	switch(arglist[ATYPE].subarg[0]){
		case 's':sprintf(query,"SELECT SongID FROM Song WHERE SongID=%d",id);break;
		case 'p':sprintf(query,"SELECT PlaylistID FROM Playlist WHERE PlaylistID=%d",id);break;
		case 'r':sprintf(query,"SELECT ArtistID FROM Artist WHERE ArtistID=%d",id);break;
		case 'a':sprintf(query,"SELECT AlbumID FROM Album WHERE AlbumID=%d",id);break;
		default:return 0;
	}
	if(doQuery(query,&dbi)==1){
		dbiClean(&dbi);
		return 1;
	}
	dbiClean(&dbi);
	return 0;
}

int *getMulti(char *arg, int *length){
	const char del[]=",";
	char *token;
	int *list,x=0;

	*length=1;
	while(arg[x]!=0){
		if(arg[x]==',')
			(*length)++;
		x++;
	}
	list=malloc(sizeof(int)*(*length));

	x=0;
	while((token=strsep(&arg,del))!=NULL){
		if(!((list[x]=(int)strtol(token,NULL,10))>0 && verifyID(list[x]))){
			if(!(list[x]=strToID(token))){
				continue;
			}
		}
		x++;
	}
	if(!x)list[0]=-1;

	return list;
}

void miClean(struct musicInfo *mi){
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
	if(mi!=NULL)
		free(mi);
}

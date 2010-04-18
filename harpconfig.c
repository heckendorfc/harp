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

#include "harpconfig.h"
#include "defs.h"
#include "insert.h"

struct lconf listconf;
struct iconf insertconf;
struct dconf debugconf;

extern struct option longopts[];

static void argConfig(char *buffer){
	char *setting=buffer;
	int y=0,argfound=0;
	while(*setting!='=' && *setting)setting++;
	if(*setting && *(setting+1))argfound=1;
	*(setting++)=0;

	for(y=0;longopts[y].name;y++){
		if(strcmp(buffer,longopts[y].name)==0){
			if(y!=AVERBOSE || !argfound)
				arglist[y].active=1;
			else
				arglist[y].active=(int)strtol(setting,NULL,10);

			if(longopts[y].has_arg>0){
				if(argfound){
					if(!(arglist[y].subarg=malloc(sizeof(char)*2)))return;
					arglist[y].subarg[0]=*setting;
					arglist[y].subarg[1]=0;
				}
				else
					arglist[y].active=0;
			}

			return;
		}
	}
}

static void listConfig(char *buffer){
	char *setting=buffer;
	int x=0,argfound=0;
	while(*setting!='=' && *setting)setting++;
	if(*setting && *(setting+1))argfound=1;
	*(setting++)=0;

	if(strcmp("width",buffer)==0){
		if(argfound)
			listconf.maxwidth=(int)strtol(setting,NULL,10);
		else // bad config. hard coded defaults
			listconf.maxwidth=30;
	}
	else if(strcmp("exception",buffer)==0){
		if(argfound)
			listconf.exception=(int)strtol(setting,NULL,10);
		else // bad config. hard coded defaults
			listconf.exception=0;
	}
}

static void insertConfig(char *buffer){
	char *setting=buffer;
	static int formatsize=0;
	int x=1,argfound=0;
	while(*setting!='=' && *setting)setting++;
	if(*setting && *(setting+1))argfound=1;
	*(setting++)=0;

	if(strcmp("usemetadata",buffer)==0 && *setting=='y'){
		if(insertconf.first_cb)
			insertconf.second_cb=&metadataInsert;
		else if(!insertconf.second_cb)
			insertconf.first_cb=&metadataInsert;
	}
	else if(strcmp("usefilepath",buffer)==0 && *setting=='y'){
		if(insertconf.first_cb)
			insertconf.second_cb=&filepathInsert;
		else if(!insertconf.second_cb)
			insertconf.first_cb=&filepathInsert;
	}
	else if(strcmp("format",buffer)==0){
		char *root=setting;
		char *ptr=setting;
		if(!(insertconf.format=realloc(insertconf.format,sizeof(char*)*(++formatsize)))
			|| !(insertconf.f_root=realloc(insertconf.f_root,sizeof(char*)*formatsize)))
			return;

		if(*ptr=='*'){
			*(insertconf.f_root+(formatsize-1))=strdup("*");
			*(insertconf.format+(formatsize-1))=strdup(ptr+1);
			*(insertconf.format+formatsize)=NULL;
			return;
		}

		while(*ptr && *(++ptr)!='%');

		*ptr=0;
		*(insertconf.f_root+(formatsize-1))=strdup(setting);
		*ptr='%';
		*(insertconf.format+(formatsize-1))=strdup(ptr);
		*(insertconf.format+formatsize)=NULL;
	}
	else if(strcmp("accuratelength",buffer)==0 && *setting=='y'){
		insertconf.length=0;
	}
}

static void debugConfig(char *buffer){
	char *setting=buffer;
	int x=0,argfound=0;
	while(*setting!='=' && *setting)setting++;
	if(*setting && *(setting+1))argfound=1;
	*(setting++)=0;

	if(strcmp("logging",buffer)==0 && *setting=='y'){
		debugconf.log=1;
	}
	else if(strcmp("directory",buffer)==0){
		if(debugconf.dir==NULL || strcmp(debugconf.dir,setting)!=0){
			if(debugconf.dir)
				free(debugconf.dir);
			debugconf.dir=strdup(setting);
		}
	}
	else if(strcmp("playlog",buffer)==0 && *setting=='y' && debugconf.dir){
		char tmp[256];
		sprintf(tmp,"%s/player.log",debugconf.dir);
		expand(tmp);
		if(debugconf.playlog)fclose(debugconf.playlog);
		debugconf.playlog=fopen(tmp,"w");
	}
	else if(strcmp("loglevel",buffer)==0 && *setting=='y'){
		debugconf.level=(int)strtol(setting,NULL,10);
		if(debugconf.level>0 && debugconf.dir){
			char tmp[256];
			sprintf(tmp,"%s/debug.log",debugconf.dir);
			expand(tmp);
			if(debugconf.msglog)fclose(debugconf.playlog);
			debugconf.msglog=fopen(tmp,"w");
		}
	}
}

static void configInit(){
	if(insertconf.format=malloc(sizeof(char*)))
		*insertconf.format=NULL;
	if(insertconf.f_root=malloc(sizeof(char*)))
		*insertconf.f_root=NULL;
	insertconf.length=-1;
	insertconf.second_cb=insertconf.first_cb=NULL;

	debugconf.msglog=debugconf.playlog=debugconf.dir=NULL;
}

static void parseConfig(FILE *ffd){
	char *buffer=malloc(255*sizeof(char));
	char *ptr=buffer;
	int x=0;
	void (*dest)(char *buffer) = NULL;

	if(!buffer)return;


	while(fgets(buffer,255,ffd)){
		if(*buffer=='['){ // New header
			switch(buffer[1]){
				case 'a':dest=&argConfig;break;
				case 'l':dest=&listConfig;break;
				case 'i':dest=&insertConfig;break;
				case 'd':dest=&debugConfig;break;
				default:dest=NULL;
			}
			continue;
		}
		if(dest && *buffer>64 && *buffer<123){
			while(*(++ptr)!='\n');
			while(*ptr<33 || *ptr>122)ptr--;
			*(ptr+1)=0;
			ptr=buffer;

			dest(buffer);
		}
	}
	free(buffer);
}

void setDefaultConfig(){
	FILE *ffd;
	char temp[250];

	configInit();

	sprintf(temp,"%s/harp/defaults.conf",SHARE_PATH);
	if((ffd=fopen(temp,"r"))==NULL){
		exit(1);
	}
	parseConfig(ffd);
	fclose(ffd);
	
	strcpy(temp,"~/.harp/defaults.conf");
	expand(temp);
	if((ffd=fopen(temp,"r"))!=NULL){
		parseConfig(ffd);
		fclose(ffd);
	}
}

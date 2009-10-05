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


void argConfig(char *buffer){
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

void listConfig(char *buffer){
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

void insertConfig(char *buffer){
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
}

int configInit(){
	if(insertconf.format=malloc(sizeof(char*)))
		*insertconf.format=NULL;
	if(insertconf.f_root=malloc(sizeof(char*)))
		*insertconf.f_root=NULL;
}

void setDefaultConfig(){
	FILE *ffd;
	char *buffer=malloc(255*sizeof(char));
	char temp[250];
	char *ptr=buffer;
	int x=0;
	void (*dest)(char *buffer) = NULL;

	if(!buffer)return;

	configInit();

	strcpy(temp,"~/.harp/defaults.conf");
	expand(temp);
	if((ffd=fopen(temp,"r"))==NULL){
		sprintf(temp,"%s/harp/defaults.conf",SHARE_PATH);
		if((ffd=fopen(temp,"r"))==NULL)
			return;
	}

	while(fgets(buffer,255,ffd)){
		if(*buffer=='['){ // New header
			switch(buffer[1]){
				case 'a':dest=&argConfig;break;
				case 'l':dest=&listConfig;break;
				case 'i':dest=&insertConfig;break;
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
	fclose(ffd);
}

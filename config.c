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
	int x=0,y,argfound=0;
	while(buffer[x]!='=' && buffer[x])x++;
	if(buffer[x] && buffer[x+1])argfound=1;
	buffer[x]=0;

	for(y=0;longopts[y].name;y++){
		if(strcmp(buffer,longopts[y].name)==0){
			if(y!=AVERBOSE || !argfound)
				arglist[y].active=1;
			else
				arglist[y].active=(int)strtol(&buffer[x+1],NULL,10);

			if(longopts[y].has_arg>0){
				if(argfound){
					arglist[y].subarg=malloc(sizeof(char)*2);
					arglist[y].subarg[0]=buffer[x+1];
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
	int x=0,argfound=0;
	while(buffer[x]!='=' && buffer[x])x++;
	if(buffer[x] && buffer[x+1])argfound=1;
	buffer[x]=0;

	if(strcmp("width",buffer)==0){
		if(argfound)
			listconf.maxwidth=(int)strtol(&buffer[x+1],NULL,10);
		else // bad config. hard coded defaults
			listconf.maxwidth=30;
	}
	else if(strcmp("exception",buffer)==0){
		if(argfound)
			listconf.exception=(int)strtol(&buffer[x+1],NULL,10);
		else // bad config. hard coded defaults
			listconf.exception=0;
	}
}

void setDefaultConfig(){
	FILE *ffd;
	char buffer[255],temp[250];
	int x=0;
	void (*dest)(char *buffer) = NULL;
	strcpy(temp,"~/.harp/defaults.conf");
	expand(temp);
	if((ffd=fopen(temp,"rb"))==NULL){
		sprintf(temp,"%s/harp/defaults.conf",SHARE_PATH);
		if((ffd=fopen(temp,"rb"))==NULL)
			return;
	}
	while(fgets(buffer,sizeof(buffer),ffd)){
		while(buffer[x]!='\n')x++;
		x=buffer[x]=0;

		if(*buffer=='['){ // New header
			switch(buffer[1]){
				case 'a':dest=&argConfig;break;
				case 'l':dest=&listConfig;break;
				default:dest=NULL;
			}
			continue;
		}
		if(dest && *buffer>64 && *buffer<123){
			dest(buffer);
		}
	}
	fclose(ffd);
}

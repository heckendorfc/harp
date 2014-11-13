/*
 *  Copyright (C) 2009-2012  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "portal.h"
#include "defs.h"
#include "util.h"

void cleanString(char *ostr){
	char *str=ostr;
	while(*str && *str!='\n')str++;
	*str=0;
	char temp[500];
	db_clean(temp,ostr,250);
	db_safe(ostr,temp,250);
	//strcpy(*ostr,temp);
}

int editWarn(char *warn){
	char c[10];
	printf("%s\nDo you wish to continue (y/n)?",warn);
	while(*(fgets(c,sizeof(c),stdin))=='\n'){
		printf("Do you wish to continue (y/n)?");
	}
	if(*c=='y' || *c=='Y')return 1;
	return 0;
}

int getStdArgs(char *args,char *prompt){
	int x;
	for(x=1;x<PORTAL_ARG_LEN && args[x] && args[x]==' ';x++);
	if(!args[x]){
		printf("%s",prompt);
		if(!fgets(args,PORTAL_ARG_LEN,stdin))return -1;
		if(*args=='\n'){
			printf("Aborted\n");
			return -1;
		}
		cleanString(args);
		return 0;
	}
	else{
		args+=x;
		cleanString(args);
		return x;
	}
}

int portal(struct commandOption *portalOptions, const char *prefix){
	char *choice;
	if(!(choice=malloc(sizeof(char)*PORTAL_ARG_LEN))){
		debug(2,"Malloc failed (portal choice).");
		return 0;
	}
	int x,ret=0;

	while(printf("%s> ",prefix) && fgets(choice,PORTAL_ARG_LEN,stdin)){
		for(x=0;choice[x] && choice[x]!='\n';x++);
		choice[x]=0;
		for(x=0;portalOptions[x].opt && portalOptions[x].opt!=*choice;x++);
		if(!portalOptions[x].opt){
			switch(*choice){
				case 'q':free(choice);return PORTAL_RET_QUIT;
				case 'p':free(choice);return PORTAL_RET_PREV;
				case '?':
				default:
					printf("Local:\n");
					for(x=0;portalOptions[x].opt;x++)printf("%c\t%s\n",portalOptions[x].opt,portalOptions[x].help);
					printf("\nGlobal:\nq\tQuit\np\tPrevious menu\n?\tPrint help\n\n");
					break;
			}
		}
		else{
			ret=portalOptions[x].function(choice,portalOptions[x].data);
			if(ret<PORTAL_RET_PREV)break;
		}
	}
	free(choice);
	return ret;
}


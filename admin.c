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

#include "admin.h"
#include "defs.h"
#include "dbact.h"
#include "util.h"
#include "portal.h"

static int addPlugin(char *args, void *data){
	char lib[200];
	int size,x;

	printf("Library (e.g., libharpmp3): ");
	size=sprintf(lib,"%s/harp/",SHARE_PATH);
	if(!fgets(&lib[size],sizeof(lib)-(size+4),stdin))return PORTAL_RET_PREV;
	for(x=size;lib[x]!='\n' && lib[x];x++);
	strcpy(&lib[x],".sql");
	debug(1,"Adding information from from file:");
	debug(1,lib);

	if(db_exec_file(lib)){
		fprintf(stderr,"Error adding plugin from file:\n\t%s\n",lib);
	}
	return PORTAL_RET_PREV;
}

static int listPlugins(char *args, void *data){
	int exception[10];
	exception[0]=exception[1]=exception[2]=exception[3]=1;

	doTitleQuery("SELECT FileType.Name AS Format, PluginType.PluginTypeID AS PluginID, PluginType.Active AS Active, Plugin.Library AS Library FROM FileType NATURAL JOIN PluginType NATURAL JOIN Plugin ORDER BY Format ASC, Active DESC",exception,1);

	return PORTAL_RET_PREV;
}

static int togglePlugin(char *args, void *data){
	char pid[50];
	char query[300];
	int x,id;

	printf("PluginID(e.g., 5): "); // Is actually PluginTypeID but this will be easier for the user to understand.
	for(x=0;args[x] && (args[x]<'0' || args[x]>'9');x++); // See if id was given as an arg
	if(!args[x]){ // Get from prompt
		if(!fgets(pid,sizeof(pid),stdin))return PORTAL_RET_PREV;
		id=(int)strtol(pid,NULL,10); // In case a number was not entered. ID 0 should not exist in the database.
	}
	else{ // Get from arg
		id=(int)strtol(&args[x],NULL,10);
		printf("%d\n",id);
	}

	sprintf(query,"UPDATE PluginType SET Active=NOT(Active) WHERE PluginTypeID=%d",id);
	sqlite3_exec(conn,query,NULL,NULL,NULL);

	return PORTAL_RET_PREV;
}

static int removePlugin(char *args, void *data){
	char lib[200];
	char query[300];
	int size,x,id;

	printf("Library (e.g., libharpmp3): ");
	if(!fgets(lib,sizeof(lib)-3,stdin))return PORTAL_RET_PREV;
	for(x=0;lib[x]!='\n' && lib[x];x++);
	strcpy(&lib[x],".so");

	sprintf(query,"SELECT PluginID FROM Plugin WHERE Library='%s' LIMIT 1",lib);
	sqlite3_exec(conn,query,uint_return_cb,&id,NULL);
	if(!id){
		printf("Library not found\n");
		return PORTAL_RET_PREV;
	}

	sprintf(query,"DELETE FROM Plugin WHERE PluginID=%d",id);
	sqlite3_exec(conn,query,NULL,NULL,NULL);
	sprintf(query,"DELETE FROM PluginType WHERE PluginID=%d",id);
	sqlite3_exec(conn,query,NULL,NULL,NULL);

	return PORTAL_RET_PREV;
}

int write_stats_cb(void *data, int col_count, char **row, char **titles){
	FILE *ffd=(FILE*)data;
	int x;
	for(x=0;x<col_count;x++){
		fputs(row[x],ffd);
		fputc('\t',ffd);
	}
	fputc('\n',ffd);
	return 0;
}

static int exportStats(char *args, void *data){
	char *num,filename[30];
	FILE *ffd;
	int x,limit;
	if((x=getStdArgs(args,"Number of songs (* for all): "))<0)return PORTAL_RET_PREV;
	num=args+x;
	debug(2,args);
	debug(2,num);
	if(*num=='*')
		sprintf(args,"SELECT SongID,Title,Location,Rating,PlayCount,SkipCount,LastPlay,Active FROM Song ORDER BY Location");
	else{
		if((limit=strtol(num,NULL,10))<1)return PORTAL_RET_PREV;
		sprintf(args,"SELECT SongID,Title,Location,Rating,PlayCount,SkipCount,LastPlay,Active FROM Song ORDER BY Location LIMIT %d",limit);
	}

	sprintf(filename,"harp_stats_%d.csv",(int)time(NULL));
	if((ffd=fopen(filename,"w"))==NULL){
		fprintf(stderr,"Failed to open file\n");
		return PORTAL_RET_PREV;
	}
	fputs("ID\tTITLE\tLOCATION\tRATING\tPLAYCOUNT\tSKIPCOUNT\tLASTPLAY\tACTIVE\n",ffd);
	debug(3,args);
	sqlite3_exec(conn,args,write_stats_cb,ffd,NULL);
	printf("Stats exported to: %s\n",filename);
	fclose(ffd);
	return PORTAL_RET_PREV;
}

static int resetAll(char *args, void *data){
	sqlite3_exec(conn,"UPDATE Song SET Rating=3,PlayCount=0,SkipCount=0,LastPlay=0",NULL,NULL,NULL);
	return PORTAL_RET_PREV;
}

static int resetRating(char *args, void *data){
	sqlite3_exec(conn,"UPDATE Song SET Rating=3",NULL,NULL,NULL);
	return PORTAL_RET_PREV;
}

static int resetPlayCount(char *args, void *data){
	sqlite3_exec(conn,"UPDATE Song SET PlayCount=0",NULL,NULL,NULL);
	return PORTAL_RET_PREV;
}

static int resetSkipCount(char *args, void *data){
	sqlite3_exec(conn,"UPDATE Song SET SkipCount=0",NULL,NULL,NULL);
	return PORTAL_RET_PREV;
}

static int resetLastPlay(char *args, void *data){
	sqlite3_exec(conn,"UPDATE Song SET LastPlay=0",NULL,NULL,NULL);
	return PORTAL_RET_PREV;
}

static int resetPortal(char *args, void *data){
	struct commandOption portalOptions[]={
		{'a',resetAll,"Reset all stats",NULL},
		{'r',resetRating,"Reset ratings",NULL},
		{'d',resetPlayCount,"Reset play count",NULL},
		{'s',resetSkipCount,"Reset skip count",NULL},
		{'l',resetLastPlay,"Reset last play time",NULL},
		{0,NULL,NULL}
	};
	return portal(portalOptions,"Reset Stats");
}

static int statsPortal(char *args, void *data){
	struct commandOption portalOptions[]={
		{'e',exportStats,"Export stats",NULL},
		{'r',resetPortal,"Reset stats",NULL},
		{0,NULL,NULL}
	};
	return portal(portalOptions,"Stats");
}

static int pluginPortal(char *args, void *data){
	struct commandOption portalOptions[]={
		{'a',addPlugin,"Add plugin",NULL},
		{'l',listPlugins,"List plugins",NULL},
		{'t',togglePlugin,"Toggle activation of plugin",NULL},
		{'r',removePlugin,"Remove plugin",NULL},
		{0,NULL,NULL}
	};
	return portal(portalOptions,"Plugins");
}

void adminPortal(){
	struct commandOption portalOptions[]={
		{'s',statsPortal,"Manage stats",NULL},
		{'p',pluginPortal,"Manage plugins",NULL},
		{0,NULL,NULL}
	};
	printf("Enter a command. ? for command list.\n");
	while(portal(portalOptions,"Admin"));
}


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


struct commandOption{
	char opt;
	int (*function)(char *args);
	const char *help;
};


int portal(struct commandOption *portalOptions, const char *prefix){
	char *choice=malloc(sizeof(char)*200);
	int x;

	while(printf("%s> ",prefix) && fgets(choice,200,stdin)){
		for(x=0;choice[x] && choice[x]!='\n';x++);
		choice[x]=0;
		for(x=0;portalOptions[x].opt && portalOptions[x].opt!=*choice;x++);
		if(!portalOptions[x].opt){
			switch(*choice){
				case 'q':free(choice);return 0;
				case 'p':free(choice);return 1;
				case '?':
				default:
					printf("Global:\nq\tQuit\np\tPrevious menu\n?\tPrint help\n\nLocal:\n");
					for(x=0;portalOptions[x].opt;x++)printf("%c\t%s\n",portalOptions[x].opt,portalOptions[x].help);
					break;
			}
		}
		else{
			if(!portalOptions[x].function(choice))break;
		}
	}
	free(choice);
	return 0;
}

int addPlugin(char *args){
	char lib[200];
	int size,x;

	printf("Library (e.g., libharpmp3): ");
	sprintf(lib,"%s/harp/plugins/%2$n",SHARE_PATH,&size);
	fgets(&lib[size],sizeof(lib)-(size+4),stdin);
	for(x=size;lib[x]!='\n' && lib[x];x++);
	strcpy(&lib[x],".sql");
	debug("Adding information from from file:");
	debug(lib);

	if(db_exec_file(lib)){
		fprintf(stderr,"Error adding plugin from file:\n\t%s\n",lib);
	}
	return 1;
}

int listPlugins(char *args){
	int *exception=alloca(sizeof(int)*10);
	struct dbitem dbi;
	dbiInit(&dbi);

	exception[0]=exception[1]=exception[2]=exception[3]=1;
	doTitleQuery("SELECT FileType.Name AS Format, PluginType.PluginTypeID AS PluginID, PluginType.Active AS Active, Plugin.Library AS Library FROM FileType NATURAL JOIN PluginType NATURAL JOIN Plugin ORDER BY Format ASC, Active DESC",&dbi,exception,1);

	return 1;
}

int togglePlugin(char *args){
	char pid[50];
	char query[300];
	int x,id;
	struct dbitem dbi;
	dbiInit(&dbi);

	printf("PluginID(e.g., 5): "); // Is actually PluginTypeID but this will be easier for the user to understand.
	for(x=0;args[x] && (args[x]<'0' || args[x]>'9');x++); // See if id was given as an arg
	if(!args[x]){ // Get from prompt
		fgets(pid,sizeof(pid),stdin);
		id=(int)strtol(pid,NULL,10); // In case a number was not entered. ID 0 should not exist in the database.
	}
	else{ // Get from arg
		id=(int)strtol(&args[x],NULL,10);
		printf("%d\n",id);
	}

	sprintf(query,"UPDATE PluginType SET Active=NOT(Active) WHERE PluginTypeID=%d",id);
	sqlite3_exec(conn,query,NULL,NULL,NULL);

	return 1;
}

int removePlugin(char *args){
	char lib[200];
	char query[300];
	int size,x,id;
	struct dbitem dbi;
	dbiInit(&dbi);

	printf("Library (e.g., libharpmp3): ");
	fgets(lib,sizeof(lib)-3,stdin);
	for(x=0;lib[x]!='\n' && lib[x];x++);
	strcpy(&lib[x],".so");

	sprintf(query,"SELECT PluginID FROM Plugin WHERE Library='%s'",lib);
	doQuery(query,&dbi);
	if(!fetch_row(&dbi)){
		printf("Library not found\n");
		return 1;
	}

	id=(int)strtol(dbi.row[0],NULL,10);

	sprintf(query,"DELETE FROM Plugin WHERE PluginID=%d",id);
	sqlite3_exec(conn,query,NULL,NULL,NULL);
	sprintf(query,"DELETE FROM PluginType WHERE PluginID=%d",id);
	sqlite3_exec(conn,query,NULL,NULL,NULL);

	dbiClean(&dbi);
	return 1;
}

int pluginPortal(char *args){
	struct commandOption portalOptions[]={
		{'a',addPlugin,"Add plugin"},
		{'l',listPlugins,"List plugins"},
		{'t',togglePlugin,"Toggle activation of plugin"},
		{'r',removePlugin,"Remove plugin"},
		{0,NULL,NULL},
	};
	return portal(portalOptions,"manage plugins");
}

void adminPortal(){
	struct commandOption portalOptions[]={
	//	{'e',exportPortal,"Export"},
	//	{'r',resetPortal,"Reset Stats"},
	//	{'l',listPortal,"List Stats"},
		{'m',pluginPortal,"Manage plugins"},
		{0,NULL,NULL},
	};
	printf("Enter a command. ? for command list.\n");
	while(portal(portalOptions,"admin"));
}


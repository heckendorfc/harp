/*
 *  Copyright (C) 2014  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "edit_shell.h"
#include "dbact.h"
#include "insert.h"

int yyparse();
TokenList *tlist;
extern commandline_t *fullcmd;
typedef int(*seltypefun)(commandline_t*);
typedef int(*acttypefun)(command_t*,command_t*);

static char *seltypes[]={
	"song",
	"album",
	"artist",
	NULL
};
void printEditSelect(int id, int type){
	char query[100];
	fprintf(stderr,"type: %d %s\n",type,seltypes[type]);
	sprintf(query,"select SelectID from TempSelect where TempID=%d",id);
	doTitleQuery(query,NULL,20);
}

struct selgroup{
	acttypefun func;
	char *name;
};

static int runGenericEditAction(commandline_t *cmd,struct selgroup *selactlist){
	int i;
	command_t *act;

	for(act=cmd->actions;act;act=act->next){
		for(i=0;selactlist[i].name;i++){
			if(strcmp(selactlist[i].name,act->cmd->word)==0){
				selactlist[i].func(cmd->selector,act);
				break;
			}
		}
	}

	return HARP_RET_ERR;
}

static int listSongs(command_t *sel, command_t *act){
	char query[300];
	int exception[10];
	int i;

	for(i=0;i<10;i++)exception[i]=i<5?1:listconf.exception;
	sprintf(query,"SELECT SongID, SongTitle, SongTrack, Location, AlbumTitle AS Album, ArtistName AS Artist FROM SongPubInfo WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",sel->tlid);
	doTitleQuery(query,exception,listconf.maxwidth);

	return HARP_RET_OK;
}

static int editSongName(command_t *sel, command_t *act){
	char query[300];

	sprintf(query,"UPDATE Song SET Name='%s' WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",act->args->words->word,sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static int deleteSong(command_t *sel, command_t *act){
	char query[300];

	sprintf(query,"DELETE FROM Song WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static int editSongAlbumArtist(command_t *sel, command_t *act){
	char query[300];
	int artistid,albumid;
	struct dbitem dbi;
	dbiInit(&dbi);

	if(act->args->words->flag==WORD_DEFAULT)
		artistid=strtol(act->args->words->word,NULL,10);
	else
		artistid=getArtist(act->args->words->word);

	if(act->args->next->words->flag==WORD_DEFAULT)
		albumid=strtol(act->args->next->words->word,NULL,10);
	else
		albumid=getAlbum(act->args->next->words->word,artistid);

	sprintf(query,"UPDATE Song SET AlbumID=%d WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",albumid,sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static int songActivation(command_t *sel, command_t *act){
	char query[300];

	/*TODO: test argument to disable toggle */
	sprintf(query,"UPDATE Song SET Active=NOT(Active) WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static int editSongLocation(command_t *sel, command_t *act){
	char query[300];

	sprintf(query,"UPDATE Song SET Location='%s' WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",act->args->words->word,sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static int editSongTrack(command_t *sel, command_t *act){
	char query[300];

	sprintf(query,"UPDATE Song SET Track=%s WHERE SongID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",act->args->words->word,sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static struct selgroup selsongacts[]={
	{listSongs,"list"},
	{editSongName,"name"},
	{editSongTrack,"track"},
	{editSongLocation,"location"},
	{editSongAlbumArtist,"albumArtist"},
	{deleteSong,"delete"},
	{songActivation,"activate"},
	{NULL,NULL}
};
static int runSongEditAction(commandline_t *cmd){
	return runGenericEditAction(cmd,selsongacts);
}

static int listAlbums(command_t *sel, command_t *act){
	char query[300];
	int exception[10];
	int i;

	for(i=0;i<10;i++)exception[i]=i<5?1:listconf.exception;
	sprintf(query,"SELECT AlbumID, Title, \"Date\" FROM Album WHERE AlbumID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",sel->tlid);
	doTitleQuery(query,exception,listconf.maxwidth);

	return HARP_RET_OK;
}
static int editAlbumDate(command_t *sel, command_t *act){
	char query[300];

	sprintf(query,"UPDATE Album SET Date=%s WHERE AlbumID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",act->args->words->word,sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static int editAlbumTitle(command_t *sel, command_t *act){
	char query[300];

	sprintf(query,"UPDATE Album SET Title='%s' WHERE AlbumID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",act->args->words->word,sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static int editAlbumArtist(command_t *sel, command_t *act){
	char query[300];
	int artistid;

	if(act->args->words->flag==WORD_DEFAULT)
		artistid=strtol(act->args->words->word,NULL,10);
	else
		artistid=getArtist(act->args->words->word);

	sprintf(query,"UPDATE AlbumArtist SET ArtistID=%d WHERE AlbumID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",artistid,sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static struct selgroup selalbumacts[]={
	{listAlbums,"list"},
	{editAlbumTitle,"title"},
	{editAlbumArtist,"artist"},
	{editAlbumDate,"date"},
	{NULL,NULL}
};
static int runAlbumEditAction(commandline_t *cmd){
	return runGenericEditAction(cmd,selalbumacts);
}

static int listArtists(command_t *sel, command_t *act){
	char query[300];
	int exception[10];
	int i;

	for(i=0;i<10;i++)exception[i]=i<5?1:listconf.exception;
	sprintf(query,"SELECT ArtistID, Name FROM Artist WHERE ArtistID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",sel->tlid);
	doTitleQuery(query,exception,listconf.maxwidth);

	return HARP_RET_OK;
}

static int editArtistName(command_t *sel, command_t *act){
	char query[300];

	sprintf(query,"UPDATE Artist SET Name='%s' WHERE ArtistID IN (SELECT SelectID FROM TempSelect WHERE TempID=%d)",act->args->words->word,sel->tlid);
	harp_sqlite3_exec(conn,query,NULL,NULL,NULL);

	return HARP_RET_OK;
}

static struct selgroup selartistacts[]={
	{listArtists,"list"},
	{editArtistName,"name"},
	{NULL,NULL}
};
static int runArtistEditAction(commandline_t *cmd){
	return runGenericEditAction(cmd,selartistacts);
}

static seltypefun selacts[]={
	runSongEditAction,
	runAlbumEditAction,
	runArtistEditAction,
	NULL
};
static int runEditAction(commandline_t *cmd){
	if(cmd->selector->tltype>=0 && cmd->selector->tltype<SEL_NULL){
		return selacts[cmd->selector->tltype](cmd);
	}
	return HARP_RET_ERR;
}

void editShell(){
	char src[512];
	int ret;

	fgets(src,512,stdin);

	tlist = lex(src);
	fullcmd=NULL;
	yyparse();

	ret=runEditAction(fullcmd);

	return;
}

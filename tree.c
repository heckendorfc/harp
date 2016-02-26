/*
 *  Copyright (C) 2009-2016  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "tree.h"
#include "defs.h"
#include "dbact.h"

struct dbnode *dbnodeAdd(struct dbnode *node){
	struct dbnode *new;
	if(!(new=malloc(sizeof(struct dbnode)))){
		debug(2,"Malloc failed (dbnode).");
		return NULL;
	}
	dbiInit(&new->dbi);
	new->prev=node;
	if(node){
		if(node->next){
			new->next=node->next;
			node->next=new;
		}
		new->depth=node->depth+1;
	}
	else{
		new->next=NULL;
		new->depth=0;
	}

	return new;
}

struct dbnode *dbnodeClean(struct dbnode *node){
	struct dbnode *ret=node->prev;
	dbiClean(&node->dbi);

//	if(node->next)
//		node->next->prev=node->prev;

	if(node->prev)
		node->prev->next=node->next;

	free(node);
	return ret;
}

// head -> parent -> ... -> 0
int *getGenreHeadPath(int head){
	int x=0;
	int *path;
	char query[100],*qptr;
	struct dbitem dbi;

	if(!(path=malloc(sizeof(int)*2))){
		debug(2,"Malloc failed (path).");
		return NULL;
	}
	dbiInit(&dbi);

	snprintf(query,100,"SELECT ParentID FROM Category WHERE CategoryID=%d",head);
	qptr=&query[47];

	path[0]=head;
	while(doQuery(query,&dbi)>0 && fetch_row(&dbi)){
		if(!(path=realloc(path,sizeof(int)*((++x)*2)))){
			debug(2,"Realloc failed (path).");
			dbiClean(&dbi);
			return NULL;
		}
		path[x]=(int)strtol(dbi.row[0],NULL,10);
		path[x+1]=0;
		snprintf(qptr,100-(qptr-query),"%d",path[x]);
	}
	dbiClean(&dbi);
	return path;
}

void printGenreHeadPath(int *path){
	int x;
	char query[100],*qptr;
	struct dbitem dbi;
	dbiInit(&dbi);

	snprintf(query,100,"SELECT Name FROM Category WHERE CategoryID=");
	qptr=&query[43];
	for(x=0;path[x];x++);
	for(x--;x>=0;x--){
		snprintf(qptr,100-(qptr-query),"%d",path[x]);
		if(doQuery(query,&dbi) && fetch_row(&dbi))
			printf("%s -> ",dbi.row[0]);
	}
	dbiClean(&dbi);
}

void printGenreChildren(struct dbnode *cur, int curid, void *action(struct dbnode*)){
	char query[100];
	int nextid;
	struct dbnode *child;

	if(!cur)return;

	snprintf(query,100,"SELECT CategoryID,Name FROM Category WHERE CategoryID=%d",curid);
	if(doQuery(query,&cur->dbi) && fetch_row(&cur->dbi))
		action(cur); // Do self

	snprintf(query,100,"SELECT CategoryID FROM Category WHERE ParentID=%d",curid);
	doQuery(query,&cur->dbi);

	while(fetch_row(&cur->dbi)){
		if(!(child=dbnodeAdd(cur)))return;
		nextid=(int)strtol(cur->dbi.row[0],NULL,10);
		if(nextid)
			printGenreChildren(child,nextid,action);
		else
			dbnodeClean(child);
	}
	dbnodeClean(cur);
}

void tierChildPrint(struct dbnode *cur){
	int x;
	char *prefix;
	int psize;
	char query[200];

	if(!cur->dbi.row_count)return;

	if(cur->depth>0){
		psize=(cur->depth*2)+1;
		if(!(prefix=malloc(sizeof(char)*psize))){
			debug(2,"Malloc failed (prefix).");
			return;
		}
		for(x=0;x<cur->depth;x++)snprintf(prefix+x,psize-x,"\t");
		printf("\n%s[%s] %s\n%s- - - - - - -\n",prefix,cur->dbi.row[0],cur->dbi.row[1],prefix);
		free(prefix);
	}
	else
		printf("\n[%s] %s\n- - - - - - -\n",cur->dbi.row[0],cur->dbi.row[1]);

	snprintf(query,200,"SELECT SongID,SongTitle,AlbumTitle,ArtistName FROM SongCategory NATURAL JOIN SongPubInfo WHERE CategoryID=%s ORDER BY ArtistName,AlbumTitle",cur->dbi.row[0]);
	doTitleQuery(query,NULL,listconf.maxwidth);
}

void tierCatPrint(struct dbnode *cur){
	int x;
	char *prefix;
	int psize;

	if(!cur->dbi.row_count)return;

	if(cur->depth>0){
		psize=(cur->depth*2)+1;
		if(!(prefix=malloc(sizeof(char)*psize))){
			debug(2,"Malloc failed (prefix).");
			return;
		}
		for(x=0;x<cur->depth;x++)snprintf(prefix+x,psize-x,"\t");
		printf("%s[%s] %s\n",prefix,cur->dbi.row[0],cur->dbi.row[1]);
		free(prefix);
	}
	else
		printf("[%s] %s\n",cur->dbi.row[0],cur->dbi.row[1]);
}

void printGenreTree(int head, void *action(struct dbnode *)){
	int x,depth,*headpath;
	struct dbitem dbi;
	struct dbnode *cur;

	dbiInit(&dbi);

	if(!(headpath=getGenreHeadPath(head))){
		fprintf(stderr,"Error in getGenreHeadPath");
		return;
	}

	for(x=0;headpath[x];x++);
	depth=x-1;
	printGenreHeadPath(headpath);
	if(headpath[0])printf("\n");
	free(headpath);

	if(!(cur=dbnodeAdd(NULL)))return;
	cur->depth=depth;
	printGenreChildren(cur,head,action);
	printf("\n");
}


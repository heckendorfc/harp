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

struct dbnode *dbnodeAdd(struct dbnode *node){
	struct dbnode *new=malloc(sizeof(struct dbnode));
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
	int *path=malloc(sizeof(int)*2);
	char query[100],*qptr;
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT ParentID FROM Category WHERE CategoryID=%d",head);
	qptr=&query[47];

	path[0]=head;
	while(doQuery(query,&dbi) && fetch_row(&dbi)){
		x++;
		path=realloc(path,sizeof(int)*(x+1));
		path[x]=(int)strtol(dbi.row[0],NULL,10);
		sprintf(qptr,"%d",path[x]);
	}

	return path;
}

void printGenreHeadPath(int *path){
	int x;
	char query[100],*qptr;
	struct dbitem dbi;
	dbiInit(&dbi);

	sprintf(query,"SELECT Name FROM Category WHERE CategoryID=");
	qptr=&query[43];
	for(x=0;path[x];x++);
	for(x--;x>=0;x--){
		sprintf(qptr,"%d",path[x]);
		if(doQuery(query,&dbi) && fetch_row(&dbi))
			printf("%s -> ",dbi.row[0]);
	}
}

void printGenreChildren(struct dbnode *cur, int curid, void *action(struct dbnode*)){
	if(!cur)return;
	char *query=alloca(sizeof(char)*100);
	sprintf(query,"SELECT CategoryID,Name FROM Category WHERE CategoryID=%d",curid);
	if(doQuery(query,&cur->dbi) && fetch_row(&cur->dbi))
		action(cur); // Do self

	sprintf(query,"SELECT CategoryID FROM Category WHERE ParentID=%d",curid);
	doQuery(query,&cur->dbi);

	int nextid;
	struct dbnode *child;
	while(fetch_row(&cur->dbi)){
		child=dbnodeAdd(cur);
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
	int *exception=alloca(sizeof(int)*10);
	for(x=1;x<10;x++)exception[x]=listconf.exception;exception[0]=1;
	if(!cur->dbi.row_count)return;
	if(cur->depth>0){
		char *prefix=alloca(sizeof(char)*((cur->depth*2)+1));
		for(x=0;x<cur->depth;x++)sprintf(&prefix[x],"\t");
		printf("\n%s[%s] %s\n%s- - - - - - -\n",prefix,cur->dbi.row[0],cur->dbi.row[1],prefix);
	}
	else
		printf("\n[%s] %s\n- - - - - - -\n",cur->dbi.row[0],cur->dbi.row[1]);

	char query[400];
	sprintf(query,"SELECT Song.SongID, Song.Title, Album.Title AS Album, Artist.Name AS Artist FROM Song,Album,Artist,AlbumArtist,SongCategory WHERE Song.AlbumID=Album.AlbumID AND Album.AlbumID=AlbumArtist.AlbumID AND AlbumArtist.ArtistID=Artist.ArtistID AND Song.SongID=SongCategory.SongID AND CategoryID=%s ORDER BY Artist.Name, Album.Title",cur->dbi.row[0]);
	doTitleQuery(query,exception,listconf.maxwidth);
}

void tierCatPrint(struct dbnode *cur){
	int x;
	if(!cur->dbi.row_count)return;
	if(cur->depth>0){
		char *prefix=alloca(sizeof(char)*((cur->depth*2)+1));
		for(x=0;x<cur->depth;x++)sprintf(&prefix[x],"\t");
		printf("%s[%s] %s\n",prefix,cur->dbi.row[0],cur->dbi.row[1],prefix);
	}
	else
		printf("[%s] %s\n",cur->dbi.row[0],cur->dbi.row[1]);
}

void printGenreTree(int head, void *action(struct dbnode *)){
	int x,depth,parent,*headpath;
	char query[150];
	struct dbitem dbi;
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

	struct dbnode *cur;
	cur=dbnodeAdd(NULL);
	cur->depth=depth;
	printGenreChildren(cur,head,action);
	printf("\n");
}


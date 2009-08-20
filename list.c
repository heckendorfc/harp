/*
 *  Copyright (C) 2009  Christian Heckendorf <vaseros@gmail.com>
 *
 *  This program is free software: you can redistribute it AND/or modify
 *  it under the terms of the GNU General Public License AS published by
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


struct dbnode{
	struct dbitem dbi;
	struct dbnode *prev;
	struct dbnode *next;
	int depth;
};

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

	if(node->next)
		node->next->prev=node->prev;

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
	if(doQuery(query,&cur->dbi)<1 || !cur->dbi.row_count)return; // Leaf node

	int nextid;
	cur=dbnodeAdd(cur);
	while(fetch_row(&cur->prev->dbi)){
		nextid=(int)strtol(cur->prev->dbi.row[0],NULL,10);
		if(nextid)
			printGenreChildren(cur,nextid,action);
	}
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
	doTitleQuery(query,&cur->dbi,exception,listconf.maxwidth);
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
	printf("\n");

	struct dbnode *cur;
	cur=dbnodeAdd(NULL);
	cur->depth=depth;
	printGenreChildren(cur,head,action);
	printf("\n");
}

int list(int *ids, int length){
	char query[401];
	int x,y,*exception=alloca(sizeof(int)*10);
	for(x=1;x<10;x++)exception[x]=listconf.exception;exception[0]=1;
	struct dbitem dbi;
	dbiInit(&dbi);
	switch(arglist[ATYPE].subarg[0]){
		case 's':
			exception[1]=exception[2]=exception[3]=exception[4]=1;
			for(x=0;x<length;x++){
				sprintf(query,"SELECT Song.SongID, Song.Title, Song.Location, Album.Title AS Album, Artist.Name AS Artist FROM Song,Album,Artist,AlbumArtist WHERE Song.AlbumID=Album.AlbumID AND Album.AlbumID=AlbumArtist.AlbumID AND AlbumArtist.ArtistID=Artist.ArtistID AND SongID=%d ORDER BY Artist.Name, Album.Title",ids[x]);
				doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			}
			break;

		case 'p':
			exception[0]=exception[1]=1;
			for(x=0;x<length;x++){
				sprintf(query,"SELECT PlaylistID,Title FROM Playlist WHERE PlaylistID=%d",ids[x]);
				doTitleQuery(query,&dbi,exception,listconf.maxwidth);
				printf("\nContents:\n");

				sprintf(query,"SELECT `Order`, Song.SongID, Song.Title, Album.Title AS Album, Artist.Name AS Artist FROM PlaylistSong,Song,Album,Artist,AlbumArtist WHERE PlaylistSong.SongID=Song.SongID AND Song.AlbumID=Album.AlbumID AND Album.AlbumID=AlbumArtist.AlbumID AND AlbumArtist.ArtistID=Artist.ArtistID AND PlaylistID=%d ORDER BY `Order`",ids[x]);
				printf("------\nTotal:%d\n",doTitleQuery(query,&dbi,exception,listconf.maxwidth));
			}
			break;

		case 'r':
			for(x=0;x<length;x++){
				sprintf(query,"SELECT Artist.ArtistID, Artist.Name FROM Artist WHERE ArtistID=%d",ids[x]);
				doTitleQuery(query,&dbi,exception,listconf.maxwidth);
				printf("\nContents:\n");

				sprintf(query,"SELECT Song.SongID, Song.Title, Album.Title AS Album, Artist.Name AS Artist FROM Song,Album,Artist,AlbumArtist WHERE Song.AlbumID=Album.AlbumID AND Album.AlbumID=AlbumArtist.AlbumID AND AlbumArtist.ArtistID=Artist.ArtistID AND Artist.ArtistID=%d ORDER BY Artist.Name, Album.Title",ids[x]);
				printf("------\nTotal:%d\n",doTitleQuery(query,&dbi,exception,listconf.maxwidth));
			}
			break;

		case 'a':
			for(x=0;x<length;x++){
				sprintf(query,"SELECT Album.AlbumID,Album.Title FROM Album WHERE AlbumID=%d",ids[x]);
				doTitleQuery(query,&dbi,exception,listconf.maxwidth);
				printf("\nContents:\n");

				sprintf(query,"SELECT Song.SongID, Song.Title, Album.Title AS Album, Artist.Name AS Artist FROM Song,Album,Artist,AlbumArtist WHERE Song.AlbumID=Album.AlbumID AND Album.AlbumID=AlbumArtist.AlbumID AND AlbumArtist.ArtistID=Artist.ArtistID AND Album.AlbumID=%d ORDER BY Artist.Name, Album.Title",ids[x]);
				printf("------\nTotal:%d\n",doTitleQuery(query,&dbi,exception,listconf.maxwidth));
			}
			break;

		case 'g':
			for(x=0;x<length;x++){
				printGenreTree(ids[x],(void *)tierChildPrint);
			}
			break;
	}
	return 0;
}

int listall(){
	char query[401];
	int x,*headpath,*exception=alloca(sizeof(int)*10);
	for(x=1;x<10;x++)exception[x]=listconf.exception;exception[0]=1;
	struct dbitem dbi;
	dbiInit(&dbi);
	switch(arglist[ATYPE].subarg[0]){
		case 's':
			sprintf(query,"SELECT Song.SongID, Song.Title, Song.Location, Album.Title AS Album, Artist.Name AS Artist FROM Song,Album,Artist,AlbumArtist WHERE Song.AlbumID=Album.AlbumID AND Album.AlbumID=AlbumArtist.AlbumID AND AlbumArtist.ArtistID=Artist.ArtistID ORDER BY Artist.Name, Album.Title");
			exception[1]=exception[2]=exception[3]=exception[4]=1;
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			break;

		case 'p':
			sprintf(query,"SELECT PlaylistID,Title FROM Playlist");
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			break;

		case 'r':
			sprintf(query,"SELECT Artist.ArtistID, Artist.Name FROM Artist");
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			break;

		case 'a':
			sprintf(query,"SELECT Album.AlbumID,Album.Title FROM Album");
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			break;

		case 'g':
			printGenreTree(0,(void *)tierCatPrint);
			break;
	}
	return 0;
}

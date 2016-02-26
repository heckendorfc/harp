/*
 *  Copyright (C) 2014-2016  Christian Heckendorf <heckendorfc@gmail.com>
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

#include <stdlib.h>
#include <string.h>
#include "build.h"
#include "edit_shell.h"
#include "util.h"
#include "dbact.h"

/* substitute vars if needed
 * escape words in quotes if needed
 * glob
 */

void free_wordchain(wordchain_t *w){
	wordchain_t *p;
	if(!w)return;
	for(p=w->next;w;){
		free(w);
		w=p;
		if(p)p=p->next;
	}
}

void free_arglist(arglist_t *a){
	arglist_t *p;
	if(!a)return;
	for(p=a->next;a;){
		free(a->words);
		free(a);
		a=p;
		if(p)p=p->next;
	}
}

void free_wordlist(wordlist_t *w){
	wordlist_t *p;
	if(!w)return;
	for(p=w->next;w;){
		free(w->word);
		free(w);
		w=p;
		if(p)p=p->next;
	}
}

void free_commands(command_t *c){
	command_t *p;
	if(!c)return;
	for(p=c->next;c;){
		free_arglist(c->args);
		free(c);
		c=p;
		if(p)p=p->next;
	}
}

wordchain_t* make_word(wordchain_t *word, char *piece, int flags){
	wordchain_t *ptr=word;
	if(piece && *piece){
		if(!word){
			INIT_MEM(word,1);
			ptr=word;
			word->word=NULL;
			word->flags=0;
			word->next=NULL;
		}
		else{
			wordchain_t *temp=ptr;
			INIT_MEM(ptr,1);
			ptr->next=temp;
		}
		ptr->word=strdup(piece);
		ptr->flags=flags;
	}
	return ptr;
}

/* default:
	subs allowed
   ":
	no subs, vars allowed
   ':
    no subs, no vars
*/
void wordcat(char *word, int *wordsize, wordchain_t *wc){
	int i;
	char *wp=word;

	while(*wp)wp++;
	if(wc->flags&(WORD_DEFAULT)){
		for(i=0;wc->word[i];i++){
			*(wp++)=wc->word[i];
		}
		*wp=0;
	}
	else if(wc->flags&(WORD_SQUOT|WORD_DQUOT)){
		for(i=0;wc->word[i];i++){
			switch(wc->word[i]){
				case '\\':
				case '*':
				case '?':
				case '[':
					*(wp++)='\\';
					break;
				case '$':
					if(wc->flags&WORD_SQUOT)
						*(wp++)='\\';
					break;
			}
			*(wp++)=wc->word[i];
		}
		*wp=0;
	}
	else
		fprintf(stderr,"Warning: invalid word flags (%s)(%d)!\n",wc->word,wc->flags);
}

char* merge_wordchain(wordchain_t *word){
	wordchain_t *wc_ptr;
	int wordsize=0;
	char *ret;

	for(wc_ptr=word;wc_ptr;wc_ptr=wc_ptr->next){
		wordsize+=strlen(wc_ptr->word);
	}

	wordsize=2*wordsize+1;
	INIT_MEM(ret,wordsize);
	*ret=0;
	for(wc_ptr=word;wc_ptr;wc_ptr=wc_ptr->next){
		wordcat(ret,&wordsize,wc_ptr);
	}

	return ret;
}

wordlist_t* append_wordlist(wordlist_t *a, wordlist_t *b){
	if(!a){
		if(b){
			return b;
		}
		return NULL;
	}
	while(a->next){
		a=a->next;
	}
	a->next=b;
	return a;
}

/* concat all words in b onto the final word in a */
wordlist_t* concat_wordlist(wordlist_t *a, wordlist_t *b){
	const char wordsep=' ';
	int count,i;
	char *c;
	wordlist_t *temp;

	if(!a || !b)return NULL; // Should do something else...

	INIT_MEM(c,1);

	while(a->next)a=a->next;

	temp=b;
	count=i=strlen(a->word);
	while(temp){
		i+=strlen(temp->word)+1;
		temp=temp->next;
	}

	INIT_MEM(c,i);
	memcpy(c,a->word,count);
	temp=b;
	for(i=0;temp;temp=temp->next){
		i=strlen(temp->word);
		memcpy(c+count,temp->word,i);
		count+=i;
		c[count++]=wordsep;
	}
	c[count-1]=0;
	if(a->word)
		free(a->word);
	a->word=c;

	return a;
}

wordlist_t* make_word_list(wordlist_t *wl, wordchain_t *word){
	wordlist_t *wl_ptr;
	if(!wl){
		INIT_MEM(wl,1);
		wl_ptr=wl;
		wl->next=NULL;
	}
	else{
		for(wl_ptr=wl;wl_ptr->next;wl_ptr=wl_ptr->next);
		INIT_MEM(wl_ptr->next,1);
		wl_ptr=wl_ptr->next;
		wl_ptr->next=NULL;
	}

	wl_ptr->word=merge_wordchain(word);
	wl_ptr->flag=word->flags; // TODO: scan all words?

	free_wordchain(word);

	return wl;
}

static char *selectors[]={
	"song",
	"album",
	"artist",
	"playlist",
	"tag",
	NULL
};

static char *selectfield[]={
	"SongID",
	"AlbumID",
	"ArtistID",
	"PlaylistID",
	"TagID",
};

static char *selecttable[]={
	"Song",
	"Album",
	"Artist",
	"Playlist",
	"Tag"
};

static char *selectcomp[]={
	"Title",
	"Title",
	"Name",
	"Title",
	"Name",
};

static char shouldmake[]={
	0,
	0,
	0,
	1,
	1,
};

static int get_select_type(command_t *c){
	int ret;

	for(ret=0;selectors[ret];ret++){
		if(strcmp(selectors[ret],c->cmd->word)==0)
			return ret;
	}
	//return -1;
	return 0; /* song default ? */
}

static int get_templist(arglist_t *a, int ctype){
	if(a->words->flag==WORD_DEFAULT){ // ID
		int id = strtol(a->words->word,NULL,10);
		return insertTempSelect(&id,1);
	}
	else{ // Name
		char query[300];
		int ret,num;
		if(a->words->flag==WORD_DQUOT)
			snprintf(query,300,"SELECT %%d,%s FROM %s WHERE %s LIKE '%%%%%s%%%%'",selectfield[ctype],selecttable[ctype],selectcomp[ctype],a->words->word);
		else
			snprintf(query,300,"SELECT %%d,%s FROM %s WHERE %s='%s'",selectfield[ctype],selecttable[ctype],selectcomp[ctype],a->words->word);
		ret=insertTempSelectQueryCount(query,&num);
		if(num==0 && shouldmake[ctype]){
			snprintf(query,300,"INSERT INTO %s(%s) VALUES(\"%s\")",selecttable[ctype],selectcomp[ctype],a->words->word);
			harp_sqlite3_exec(conn,query,NULL,NULL,NULL);
			num=sqlite3_last_insert_rowid(conn);
			return insertTempSelect(&num,1);
			//sprintf(query,"SELECT %%d,%s FROM %s WHERE %s LIKE '%%%%%s%%%%'",selectfield[ctype],selecttable[ctype],selectcomp[ctype],a->words->word);
			//return insertTempSelectQuery(query);
		}
		return ret;
	}
}

static char *translatequery[]={
	NULL,//ss
	"SELECT %%d,AlbumID FROM Song WHERE SongID IN (%s)",//sa (song -> album)
	"SELECT %%d,ArtistID FROM AlbumArtist NATURAL JOIN Song WHERE SongID IN (%s)",//sr
	"SELECT %%d,PlaylistID FROM PlaylistSong WHERE SongID IN (%s)",//sp
	"SELECT %%d,TagID FROM SongTag WHERE SongID IN (%s)",//st

	"SELECT %%d,SongID FROM Song WHERE AlbumID IN (%s)",//as
	NULL,//aa
	"SELECT %%d,ArtistID FROM AlbumArtist WHERE AlbumID IN (%s)",//ar
	"SELECT %%d,PlaylistID FROM PlaylistSong NATURAL JOIN Song WHERE AlbumID IN (%s)",//ap
	"SELECT %%d,TagID FROM SongTag NATURAL JOIN Song WHERE AlbumID IN (%s)",//at

	"SELECT %%d,SongID FROM Song NATURAL JOIN AlbumArtist WHERE ArtistID IN (%s)",//rs
	"SELECT %%d,AlbumID FROM AlbumArtist WHERE ArtistID IN (%s)",//ra
	NULL,//rr
	"SELECT %%d,PlaylistID FROM PlaylistSong NATURAL JOIN Song NATURAL JOIN AlbumArtist WHERE IN ArtistID(%s)",//rp
	"SELECT %%d,TagID FROM SongTag NATURAL JOIN Song NATURAL JOIN AlbumArtist WHERE ArtistID IN (%s)",//rt

	"SELECT %%d,SongID FROM PlaylistSong WHERE PlaylistID IN (%s)",//ps
	"SELECT %%d,AlbumID FROM PlaylistSong NATURAL JOIN Song WHERE PlaylistID IN (%s)",//pa
	"SELECT %%d,ArtistID FROM PlaylistSong NATURAL JOIN Song NATURAL JOIN AlbumArtist WHERE PlaylistID IN (%s)",//pr
	NULL,//pp
	"SELECT %%d,TagID FROM SongTag NATURAL JOIN PlaylistSong WHERE PlaylistID IN (%s)",//pt

	"SELECT %%d,SongID FROM SongTag WHERE TagID IN (%s)",//ts
	"SELECT %%d,AlbumID FROM SongTag NATURAL JOIN Song WHERE TagID IN (%s)",//ta
	"SELECT %%d,ArtistID FROM SongTag NATURAL JOIN Song NATURAL JOIN AlbumArtist WHERE TagID IN (%s)",//tr
	"SELECT %%d,PlaylistID FROM SongTag NATURAL JOIN PlaylistSong WHERE TagID IN (%s)",//tp
	NULL,//tt
};
static const int numtypes=5;

static int translate_templist(int toid, int totype, int fromid, int fromtype, int cleanup){
	char query[300];
	char subquery[300];
	int newid;
	if(toid<0 && totype==fromtype)
		return fromid;
	else{
		if(totype==fromtype){
			mergeTempSelect(toid,fromid);
			newid=toid;
		}
		else{
			snprintf(subquery,300,"SELECT SelectID FROM TempSelect WHERE TempID=%d",fromid);
			snprintf(query,300,translatequery[fromtype*numtypes+totype],subquery);
			newid=insertTempSelectQuery(query);
			//mergeTempSelect(toid,newid);
		}
	}

	if(cleanup)
		cleanTempSelect(fromid);

	return newid;
}

arglist_t* make_com_arg(void *data, int flag){
	arglist_t *ret;

	INIT_MEM(ret,1);
	ret->flags=flag;
	ret->next=NULL;

	if(flag==COM_ARG_SELECTOR){
		command_t *c = (command_t*)data;
		arglist_t *a = c->args;
		int atid;
		int cid=-1;

		ret->tltype=c->tltype;

		if(c->tlid>=0)
			cid=c->tlid;
		else{
			while(a){
				atid=get_templist(a,c->tltype);

				if(cid==-1)
					cid=atid;
				else
					mergeTempSelect(cid,atid);

				a=a->next;
			}
		}

		ret->words=NULL;
		ret->tlid=cid;
	}
	else{
		wordlist_t *wl = (wordlist_t*)data;
		ret->words=wl;
		ret->tlid=-1;
		ret->tltype=-1;
	}

	//ret->args=wl;
	//fprintf(stderr,"arg: %s\n",wl->word);

	return ret;
}

arglist_t* append_com_arg(arglist_t *a, arglist_t *b){
	if(b){
		b->next=a;
		return b;
	}
	return a;
}

void com_sel_set_args(command_t *c, arglist_t *a){
	arglist_t *ta;
	int cid;
	ta=c->args=a;

	while(ta){
		if(ta->tltype<0) // single native
			cid=ta->tlid=get_templist(ta,c->tltype);
		else
			cid=translate_templist(c->tlid,c->tltype,ta->tlid,ta->tltype,1);

		if(c->tlid<0)
			c->tlid=cid;
		else
			mergeTempSelect(c->tlid,cid);

		ta=ta->next;
	}
}

void com_act_set_args(command_t *c, arglist_t *a){
	c->args=a;
}

command_t* com_set_args(command_t *c, arglist_t *a, int flag){
	c->flags=flag;

	//if(flag==COM_SEL)
		com_sel_set_args(c,a);
	//else
		//com_act_set_args(c,a);
	return c;
}

command_t* make_command(wordlist_t *wl){
	command_t *ret;

	INIT_MEM(ret,1);
	ret->cmd=wl;
	ret->args=NULL;
	ret->flags=0;
	ret->next=NULL;
	ret->tltype=get_select_type(ret);
	ret->tlid=-1;

	return ret;
}

command_t* append_command(command_t *a, command_t *b){
	command_t *temp=a;

	while(a->next)a=a->next;
	a->next=b;

	return temp;
}

void append_command_flags(command_t *a, const int flags){
	while(a->next)a=a->next;
	a->flags|=flags;
}

commandline_t* make_commandline(command_t *sel, command_t *act){
	commandline_t *ret;

	INIT_MEM(ret,1);
	ret->selector=sel;
	ret->actions=act;

	return ret;
}

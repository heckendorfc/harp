#include <stdlib.h>
#include <string.h>
#include "build.h"
#include "edit_shell.h"
#include "util.h"

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
		free(a->args);
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
	int wordlen;
	wordchain_t *ptr=word;
	if(piece && *piece){
		if(!word){
			INIT_MEM(word,1);
			ptr=word;
			word->word=NULL;
			word->flags=0;
			word->next=NULL;
			wordlen=0;
		}
		else{
			wordchain_t *temp=ptr;
			INIT_MEM(ptr,1);
			ptr->next=temp;
		}
		ptr->word=piece;
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
	NULL
};

static char *selectfield[]={
	"SongID",
	"AlbumID",
	"ArtistID"
};

static char *selecttable[]={
	"Song",
	"Album",
	"Artist"
};

static char *selectcomp[]={
	"Title",
	"Title",
	"Name"
};

static int get_select_type(command_t *c){
	int ret;

	for(ret=0;selectors[ret];ret++){
		if(strcmp(selectors[ret],c->cmd->word)==0)
			return ret;
	}
	return -1;
}

static int get_templist(arglist_t *a, int ctype){
	if(a->args->flag==WORD_DEFAULT){ // ID
		int id = strtol(a->args->word,NULL,10);
		return insertTempSelect(&id,1);
	}
	else{ // Name
		char query[300];
		sprintf(query,"SELECT %s FROM %s WHERE %s LIKE '%%%s%%'",selectfield[ctype],selecttable[ctype],selectcomp[ctype],a->args->word);
		return insertTempSelectQuery(query);
	}
}

static char *translatequery[]={
	NULL,//ss
	"SELECT %%d,AlbumID FROM Song WHERE SongID IN (%s)",//sa (song -> album)
	"SELECT %%d,ArtistID from AlbumArtist NATURAL JOIN Song WHERE SongID IN (%s)",//sr

	"SELECT %%d,SongID FROM Song WHERE AlbumID IN (%s)",//as
	NULL,//aa
	"SELECT %%d,ArtistID FROM AlbumArtist WHERE AlbumID IN (%s)",//ar

	"SELECT %%d,SongID FROM Song NATURAL JOIN AlbumArtist WHERE ArtistID IN (%s)",//rs
	"SELECT %%d,AlbumID FROM AlbumArtist WHERE ArtistID IN (%s)",//ra
	NULL,//rr
};
static const int numtypes=3;

static int translate_templist(int toid, int totype, int fromid, int fromtype, int cleanup){
	char query[300];
	char subquery[300];
	int newid;
	if(toid<0){ // new id
		if(totype==fromtype)
			return fromid;
	}
	else{
		if(totype==fromtype){
			mergeTempSelect(toid,fromid);
			return toid;
		}

		sprintf(subquery,"SELECT SelectID FROM TempSelect WHERE TempID=%d",fromid);
		sprintf(query,translatequery[fromtype*numtypes+totype],subquery);
		newid=insertTempSelectQuery(query);
		//mergeTempSelect(toid,newid);
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

		while(a){
			atid=get_templist(a,c->tltype);

			if(cid==-1)
				cid=atid;
			else
				mergeTempSelect(cid,atid);

			a=a->next;
		}

		ret->tlid=cid;
	}
	else{
		wordlist_t *wl = (wordlist_t*)data;
		ret->args=wl;
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
			cid=translate_templist(ta->tlid,ta->tltype,c->tlid,c->tltype,1);

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

	if(flag==COM_SEL)
		com_sel_set_args(c,a);
	else
		com_act_set_args(c,a);

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

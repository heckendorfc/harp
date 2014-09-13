#include <build.h>
#include <edit_shell.h>

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

void free_redirection(redirect_t *r){
	redirect_t *p;
	if(!r)return;
	for(p=r->next;r;){
		free(r);
		r=p;
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
		free_wordlist(c->args);
		free_redirection(c->redirection);
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

	free_wordchain(word);

	return wl;
}

redirect_t* make_redirect(int type, wordchain_t *fd, wordchain_t *dest){
	redirect_t *ret;
	char *word;

	INIT_MEM(ret,1);
	//ret->wc_fd=fd;
	//ret->wc_dest=dest;
	ret->flags=0;
	ret->next=NULL;


	if(fd!=NULL){
		word=merge_wordchain(fd);
		if(*word>='0' && *word<='9')
			ret->fd=strtol(word,NULL,10);
		free(word);
	}
	else
		ret->fd=-1;
	ret->dest=merge_wordchain(dest);
	ret->d_flag=REDIR_DEST_STR;

	free_wordchain(fd);
	free_wordchain(dest);

	switch(type){
		case TOK_LT:
			ret->flags = O_RDONLY;
			if(ret->fd<0)ret->fd=0;
			break;

		case TOK_GT:
			ret->flags = O_TRUNC | O_WRONLY | O_CREAT;
			if(ret->fd<0)ret->fd=1;
			break;

		case TOK_GTAMP:
			ret->flags = O_WRONLY;
			if(ret->fd<0 || ret->fd>9)ret->fd=1;
			if(*ret->dest>='0' && *ret->dest<='9' && ret->dest[1]=='\0'){
				char *temp = ret->dest;
				INIT_MEM(ret->dest,10); /* /dev/fd/?\0 */
				sprintf(ret->dest,"/dev/fd/%c",*temp);
				free(temp);
			}
			else{
				char *temp = ret->dest;
				INIT_MEM(ret->dest,10); /* /dev/fd/?\0 */
				sprintf(ret->dest,"/dev/fd/%d",ret->fd);
				free(temp);
			}
			break;

		case TOK_GTGT:
			ret->flags = O_APPEND | O_WRONLY | O_CREAT;
			if(ret->fd<0)ret->fd=1;
			break;

		default:
			fprintf(stderr,"Warning: using default redirect flags\n"); break;
	}

	return ret;
}

command_t* make_command(wordlist_t *wl, redirect_t *redirect){
	command_t *ret;

	INIT_MEM(ret,1);
	ret->args=wl;
	ret->redirection=redirect;
	ret->flags=0;
	ret->next=0;
	ret->exec.infd=ret->exec.outfd=-1;
	ret->exec.pid=0;

	return ret;
}

command_t* make_for_command(wordlist_t *var, wordlist_t *list, command_t *c){
	command_t *ret;

	INIT_MEM(ret,1);

	ret->args=var;
	free_wordlist(var->next);
	var->next=list;

	ret->next=c;

	ret->redirection=NULL;
	ret->flags=0;
	ret->exec.infd=ret->exec.outfd=-1;
	ret->exec.pid=0;

	return ret;
}

command_t* make_while_command(command_t *testc, command_t *runc){
	command_t *ret;

	INIT_MEM(ret,1);

	/* blank -> {test} -> {run}|end */
	ret->next=testc;
	append_command_flags(ret->next,COM_SEMI);
	append_command(ret->next,runc);
	append_command(ret->next,make_command(NULL,NULL));
	append_command_flags(ret->next,COM_ENDWHILE);

	ret->redirection=NULL;
	ret->flags=COM_SEMI|COM_WHILE;
	ret->exec.infd=ret->exec.outfd=-1;
	ret->exec.pid=0;

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

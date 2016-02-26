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

#include "lex_dfa.h"
#include "lex.h"
#include "edit_shell.h"

void free_tokens(TokenList *t){
	TokenList *p;
	if(!t)return;
	for(p=t->next;t;){
		if(t->token.word)
			free(t->token.word);
		free(t);
		t=p;
		if(p)p=p->next;
	}
}

STATIC
int split_token(TokenList *token, const int start, const int word_i){
	const int len=strlen(token->token.word);
	int count=0;
	TokenList *orig,*ptr;
	orig=token;

	if(start==0 && word_i==len)
		return 0;

	if(start>0){
		count|=SPLIT_BEFORE;
		ptr=token->next;
		INIT_MEM(token->next,1);
		token->next->next=ptr;
		INIT_MEM(token->next->token.word,(word_i-start)+1);
		memcpy(token->next->token.word,orig->token.word+start,word_i-start);
		token->next->token.word[word_i-start]=0;
		token=token->next;
	}
	if(word_i<len){
		count|=SPLIT_AFTER;
		ptr=token->next;
		INIT_MEM(token->next,1);
		token->token.type=orig->token.type;
		token->next->token.type=TOK_NULL;
		token->next->next=ptr;
		INIT_MEM(token->next->token.word,(len-word_i)+1);
		memcpy(token->next->token.word,orig->token.word+word_i,len-word_i);
		token->next->token.word[len-word_i]=0;
	}

	if(start>0){
		orig->token.word[start]=0;
		orig->next->token.type=orig->token.type;
		orig->token.type=TOK_NULL;
	}
	else{
		orig->token.word[word_i]=0;
	}

	return count;
}

STATIC
int identify(TokenList *token,State *q){
	int i,c=0;
	char *str=token->token.word;
	State *test=q;
	int candidate[NUM_CANDIDATE];
	int tok[NUM_CANDIDATE];
	int tok_start=-1;
	//int l_tok=TOK_NULL;

	if(!str || !*str)
		return 0;

	for(i=0;str[i];i++){
		if(str[i]=='\\'){
			if(c>0) /* We don't look for TEXT tokens */
				break;
			else{
				i++;
				continue;
			}
		}

		test=test->out[(int)str[i]].state;

		if(!test)break;

		if(tok_start<0 && test!=q)
			tok_start=i;

		if(test->final){
			candidate[c]=i;
			tok[c]=test->final;
			c++;

			assert(c<NUM_CANDIDATE);
		}
		if(test->out==NULL){
			/* No more processing can be done on this segment */
			break;
		}
	}
	if(c>0){
		token->token.type=tok[c-1];
		int split=split_token(token,tok_start,candidate[c-1]+1);
		if(split==(SPLIT_BEFORE|SPLIT_AFTER)){
			identify(token->next->next,q);
		}
		else if(split&SPLIT_AFTER){
			identify(token->next,q);
		}
		return split;
	}
	return 0;
}

#if 0
STATIC
int identify_full(TokenList *token, State *q){
	State *test=q;
	char *str=token->token.word;
	int i;

	if(!str || !*str)
		return 0;

	for(i=0;str[i];i++){
		if(!test->out)
			return 0;

		test=test->out[(int)str[i]].state;

		if(!test)
			return 0;
	}
	if(test->final)
		token->token.type=test->final;

	return test->final;
}

STATIC
void strip_backslash(Token *token){
	int i,j;
	/*TODO: replace with special backslash chars (\n) */
	for(i=0;token->word[i];i++){
		if(token->word[i]=='\\'){
			for(j=i;token->word[j];j++)
				token->word[j]=token->word[j+1];
		}
	}
}
#endif

STATIC
TokenList* create_tokens(char *str){
	TokenList *list,*t;
	char *start,*ptr=str;
	char quote=0;
	int escape=0;

	INIT_MEM(list,1);
	t=list;
	start=ptr;
	for(;*ptr;ptr++){
		if(!escape && *ptr=='\\'){
			escape=1;
			ptr++;
			if(*ptr==0)break;
		}

		if(!escape && quote==0 && (*ptr=='"' || *ptr=='\'')){ /* Found initial quote */
			quote=*ptr;
			*ptr=0;
			ptr++;

			if(*start){
				t->token.word=strdup(start);
				t->token.type=TOK_NULL;
				INIT_MEM(t->next,1);
				t=t->next;
			}

			INIT_MEM(t->token.word,2);
			snprintf(t->token.word,2,"%c",quote);
			t->token.type=TOK_QUOTE;
			INIT_MEM(t->next,1);
			t=t->next;

			if(*ptr==0)break;

			start=ptr;

			continue;
		}

		if(!escape && quote && *ptr==quote){ /* Found matching quote */
			*ptr=0;
			if(*start){
				t->token.word=strdup(start);
				t->token.type=TOK_QUOTE_STR;
				INIT_MEM(t->next,1);
				t=t->next;
			}

			INIT_MEM(t->token.word,2);
			snprintf(t->token.word,2,"%c",quote);
			t->token.type=TOK_QUOTE;
			INIT_MEM(t->next,1);
			t=t->next;

			start=ptr+1;
			quote=0;

			continue;
		}

		if(!escape && quote==0 && (*ptr==' ' || *ptr=='\t' || *ptr=='\n')){
			*ptr=0;

			if(*start){
				t->token.word=strdup(start);
				t->token.type=TOK_NULL;
				INIT_MEM(t->next,1);
				t=t->next;
			}

			t->token.word=strdup(" ");
			t->token.type=TOK_WHITESPACE;
			INIT_MEM(t->next,1);
			t=t->next;

			while(ptr[1]==' ' || ptr[1]=='\t' || ptr[1]=='\n'){
				ptr++;
			}
			start=ptr+1;
		}

		if(escape) escape=0;
	}

	if(*start){
		t->token.word=strdup(start);
		t->token.type=TOK_NULL;
		t->next=NULL;
	}
	else{
		TokenList* tp=list;
		while(tp->next!=t)tp=tp->next;
		free(tp->next);
		tp->next=NULL;
	}

	return list;
}

TokenList* lex(const char *str){
	char *tofree,*strp;
	static State *op_dfa=NULL;
	//static State *reserved_dfa=NULL;
	TokenList *tokens,*tptr;

	if(!op_dfa)op_dfa=generate_operator_dfa();
	//if(!reserved_dfa)reserved_dfa=generate_reserved_dfa();

	tofree=strp=strdup(str);

	INIT_MEM(tptr,1);
	tokens=tptr;
	tptr->token.word=NULL;
	tptr->token.type=TOK_NULL;
	tptr->next=NULL;

	tptr=tptr->next=create_tokens(strp);

	for(tptr=tokens->next;tptr!=NULL;tptr=tptr->next){
		if(tptr->token.type!=TOK_NULL)continue;
		identify(tptr,op_dfa);
	}
	/*
	for(tptr=tokens->next;tptr!=NULL;tptr=tptr->next){
		if(tptr->token.type!=TOK_NULL)continue;
		identify_full(tptr,reserved_dfa);
	}
	*/
	for(tptr=tokens->next;tptr!=NULL;tptr=tptr->next){
		if(tptr->next && tptr->next->token.type==TOK_WHITESPACE &&
		   tptr->next->next && tptr->next->next->token.type&(TOK_OPERATOR|TOK_RESERVED)){
				TokenList *temp=tptr->next->next;
				free(tptr->next->token.word);
				free(tptr->next);
				tptr->next=temp;
		}
		if(tptr->next && tptr->token.type&(TOK_OPERATOR|TOK_RESERVED) && tptr->next->token.type==(TOK_WHITESPACE)){
				TokenList *temp=tptr->next->next;
				free(tptr->next->token.word);
				free(tptr->next);
				tptr->next=temp;
		}
	}
	for(tptr=tokens->next;tptr!=NULL;tptr=tptr->next){
		if(tptr->token.type==TOK_NULL)
			tptr->token.type=TOK_TEXT;
	}

	free(tofree);

	return tokens;
}

int yyerror(const char *str){
	fprintf(stderr,"%s\n",str);
	return 0;
}

int yylex(){
	int ret;

	if(!tlist || !tlist->next)return 0;

	for(;tlist->next && tlist->token.type==TOK_WHITESPACE && tlist->next->token.type==TOK_WHITESPACE;tlist=tlist->next);

	tlist=tlist->next;

	if(tlist->token.type==TOK_WHITESPACE && tlist->next==NULL)
		return 0;

	ret=TOKEN_MASK&tlist->token.type;
	//printf("YYLEX|%d|%s\n",ret,tlist->token.word);
	yylval.word=tlist->token.word;

	return ret;
}

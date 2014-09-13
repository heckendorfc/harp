#ifndef LEX_H
#define LEX_H

/* Maxumum word length */
#define NUM_CANDIDATE 5

enum SplitPlace{
	SPLIT_BEFORE=1,
	SPLIT_AFTER=2,
};

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <lex_dfa.h>


typedef struct Token{
	char *word;
	int type;
}Token;

typedef struct TokenList{
	Token token;
	struct TokenList *next;
}TokenList;


#ifdef TEST_MODE
#define STATIC
int split_token(TokenList *token, const int start, const int word_i);
int identify(TokenList *token,State *q);
int identify_full(TokenList *token, State *q);
void strip_backslash(Token *token);
TokenList* create_tokens(char *str);
#else
#define STATIC static
#endif

void free_tokens(TokenList *t);
TokenList* lex(const char *str);

#endif

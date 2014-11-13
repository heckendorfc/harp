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

#include "lex_dfa.h"


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

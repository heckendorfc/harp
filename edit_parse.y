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
%{
#include <stdio.h>
#include "lex.h"
#include "build.h"
int yylex();
int yyerror();

commandline_t *fullcmd;
int charsub=0;
int wp_flag=0;
%}

%token TOK_TEXT TOK_QUOTE TOK_QUOTE_STR TOK_WHITESPACE TOK_COMMA
%token TOK_WORD
%token TOK_SEMICOLON TOK_OPAR TOK_CPAR

%union{
	int number;
	char *word;
	wordchain_t *wc;
	wordlist_t *wl;
	arglist_t *arg;
	command_t *command;
	commandline_t *commandline;
}

%type<word> word_piece TOK_TEXT TOK_QUOTE_STR TOK_QUOTE
%type<wc> text_word word
%type<command> cmd selector action actionlist
%type<commandline> cmdline
%type<arg> onearg arglist

%start cmdline

%%

word_piece:	TOK_TEXT
			{
				wp_flag=WORD_DEFAULT;
				$$=$1;
			}
	|	TOK_QUOTE TOK_QUOTE_STR TOK_QUOTE
			{
				wp_flag=(*($1)=='"'?WORD_DQUOT:WORD_SQUOT);
				$$=$2;
			}
	;

text_word: TOK_TEXT
			{ $$ = make_word(NULL,$1,WORD_DEFAULT); }

word:	word word_piece
			{ $$ = make_word($1,$2,wp_flag); }
	|	word_piece
			{ $$ = make_word(NULL,$1,wp_flag); }
	;

cmd:	text_word TOK_OPAR
			{ $$ = make_command(make_word_list(NULL,$1)); }

onearg:	word
			{ $$ = make_com_arg(make_word_list(NULL,$1),COM_ARG_LITERAL); }
	|	selector
			{ $$ = make_com_arg($1,COM_ARG_SELECTOR); }
	;

arglist:	onearg
			{ $$ = $1; }
	|	arglist TOK_COMMA onearg
			{ $$ = append_com_arg($3,$1); }
	;

selector:	cmd arglist TOK_CPAR
			{ $$ = com_set_args($1,$2,COM_SEL); }

action:	cmd arglist TOK_CPAR
			{ $$ = com_set_args($1,$2,COM_ACT); }
	|	text_word
			{ $$ = make_command(make_word_list(NULL,$1)); $$->flags=COM_ACT; }
	;

actionlist:	action
			{ $$ = $1; }
	|	actionlist action
			{ $$ = append_command($1,$2); }
	|	actionlist TOK_WHITESPACE action
			{ $$ = append_command($1,$3); }
	;

cmdline:	selector actionlist
			{ fullcmd = make_commandline($1,$2); }

%{
#include <stdio.h>
#include "lex.h"
#include "build.h"
int yylex();
int yyerror();

command_t *fullcmd;
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
}

%type<word> word_piece TOK_TEXT TOK_QUOTE_STR TOK_QUOTE
%type<wc> word
%type<command> cmd selector action cmdline
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

word:	word word_piece
			{ $$ = make_word($1,$2,wp_flag); }
	|	word_piece
			{ $$ = make_word(NULL,$1,wp_flag); }
	;

cmd:	word TOK_OPAR
			{ $$ = make_command(make_word_list(NULL,$1)); }

onearg:	word
			{ $$ = make_com_arg(make_word_list(NULL,$1),COM_ARG_LITERAL); }
	|	selector
			{ $$ = make_com_arg($1,COM_ARG_SELECTOR); }
	;

arglist:	onearg
			{ $$ = $1; }
	|	arglist TOK_COMMA onearg
			{ $$ = append_com_arg($1,$3); }
	;

selector:	cmd arglist TOK_CPAR
			{ $$ = com_set_args($1,$2,COM_SEL); }

action:	cmd arglist TOK_CPAR
			{ $$ = com_set_args($1,$2,COM_ACT); }
	|	word
			{ $$ = make_command(make_word_list(NULL,$1)); $$->flags=COM_ACT; }
	;

cmdline:	selector action
			{ fullcmd = $2; $$ = $1; fullcmd->tlid = $1.tlid }

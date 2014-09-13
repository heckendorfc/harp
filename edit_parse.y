%{
#include <stdio.h>
#include <lex.h>
#include <build.h>
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
	command_t *command;
}

%start cmdline

%%

word_piece:	TOK_TEXT
			{
				wp_flag=WORD_DEFAULT;
				$$.word=$1.word;
			}
	|	TOK_QUOTE TOK_QUOTE_STR TOK_QUOTE
			{
				wp_flag=(*($1.word)=='"'?WORD_DQUOT:WORD_SQUOT);
				$$.word=$2.word;
			}
	;

word:	word word_piece
			{ $$.wc = make_word($1.wc,$2.word,wp_flag); }
	|	word_piece
			{ $$.wc = make_word(NULL,$1.word,wp_flag); }
	;

cmd:	word TOK_OPAR
			{ $$.command = make_command(make_word_list($1)); }

onearg:	word
			{ $$ = make_com_arg(make_word_list($1),COM_ARG_LITERAL); }
	|	selector
			{ $$ = make_com_arg($1,COM_ARG_SELECTOR); }
	;

arglist:	onearg
			{ $$ = $1; }
	|	arglist TOK_COMMA onearg
			{ $$ = append_com_arg($1,$3); }
	;

selector:	cmd arglist TOK_CPAR
			{ $$.command = com_set_args($1,$2); $$.command->flags=COM_SEL; }

action:	cmd arglist TOK_CPAR
			{ $$.command = com_set_args($1,$2); $$.command->flags=COM_ACT; }
	|	word
			{ $$.command = make_command(make_word_list($1)); $$.command->flags=COM_ACT; }
	;

cmdline:	selector action
			{ fullcmd = $1.command; $$ = $1; }

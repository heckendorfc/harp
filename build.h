#ifndef BUILD_H
#define BUILD_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#define WORD_DEFAULT	1
#define WORD_SQUOT		2
#define WORD_DQUOT		4

#define COM_ARG_LITERAL 1
#define COM_ARG_SELECTOR 2

#define COM_SEL 1
#define COM_ACT 2

typedef struct wordchain_t{
	char *word;
	int flags;
	struct wordchain_t *next;
}wordchain_t;

typedef struct wordlist_t{
	char *word;
	int flag;
	struct wordlist_t *next;
}wordlist_t;

typedef struct arglist_t{
	wordlist_t *words;
	int flags;
	int tlid;
	int tltype;
	struct arglist_t *next;
}arglist_t;

typedef struct command_t{
	wordlist_t *cmd;
	arglist_t *args;
	int flags;
	int tlid;
	int tltype;
	struct command_t *next;
}command_t;

typedef struct commandline_t{
	command_t *selector;
	command_t *actions;
}commandline_t;

void free_wordchain(wordchain_t *w);
void free_wordlist(wordlist_t *w);
void free_arglist(arglist_t *a);
void free_commands(command_t *c);
wordchain_t* make_word(wordchain_t *word, char *piece, int flags);
wordlist_t* make_word_list(wordlist_t *wl, wordchain_t *word);
wordlist_t* append_wordlist(wordlist_t *a, wordlist_t *b);
wordlist_t* concat_wordlist(wordlist_t *a, wordlist_t *b);
//arglist_t* make_com_arg(wordlist_t *wl, int flag);
arglist_t* make_com_arg(void *data, int flag);
arglist_t* append_com_arg(arglist_t *a, arglist_t *b);
command_t* com_set_args(command_t *c, arglist_t *a, int flag);
command_t* make_command(wordlist_t *wl);
commandline_t* make_commandline(command_t *sel, command_t *act);
command_t* append_command(command_t *a, command_t *b);
void append_command_flags(command_t *a, const int flags);

#endif

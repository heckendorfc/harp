#ifndef BUILD_H
#define BUILD_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#define WORD_DEFAULT	1
#define WORD_SQUOT		2
#define WORD_DQUOT		4

#define COM_SEMI		0x001
#define COM_PIPE 		0x002
#define COM_BG 			0x004
#define COM_SUBST		0x008
#define COM_ENDFOR		0x010
#define COM_ENDWHILE	COM_ENDFOR
#define COM_AND			0x020
#define COM_OR			0x040

#define COM_DEFAULT 	0x100
#define COM_VAR 		0x200
#define COM_FOR 		0x400
#define COM_WHILE 		0x800

#define COM_BASE_MASK		0xF00
#define COM_TERM_MASK		0x0FF

#define REDIR_DEST_STR 	1
#define REDIR_DEST_INT 	2

typedef struct wordchain_t{
	char *word;
	int flags;
	struct wordchain_t *next;
}wordchain_t;

typedef struct wordlist_t{
	char *word;
	struct wordlist_t *next;
}wordlist_t;

typedef struct redirect_t{
	//wordchain_t *wc_fd;
	//wordchain_t *wc_dest;
	int fd;
	union {
		char *dest;
		int dfd;
	};
	int d_flag;
	int flags;
	struct redirect_t *next;
}redirect_t;

typedef struct command_t{
	wordlist_t *args;
	redirect_t *redirection;
	int flags;
	struct{
		int pid;
		int infd;
		int outfd;
	}exec;
	struct command_t *next;
}command_t;

void free_wordchain(wordchain_t *w);
void free_redirection(redirect_t *r);
void free_wordlist(wordlist_t *w);
void free_commands(command_t *c);
wordchain_t* make_word(wordchain_t *word, char *piece, int flags);
wordlist_t* make_word_list(wordlist_t *wl, wordchain_t *word);
wordlist_t* append_wordlist(wordlist_t *a, wordlist_t *b);
wordlist_t* concat_wordlist(wordlist_t *a, wordlist_t *b);
redirect_t* make_redirect(int type, wordchain_t *in, wordchain_t *out);
command_t* make_command(wordlist_t *wl, redirect_t *redirect);
command_t* make_for_command(wordlist_t *var, wordlist_t *list, command_t *c);
command_t* make_while_command(command_t *testc, command_t *runc);
command_t* append_command(command_t *a, command_t *b);
void append_command_flags(command_t *a, const int flags);

#endif

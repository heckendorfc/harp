#ifndef EDIT_SHELL_H
#define EDIT_SHELL_H

#include "lex.h"
#include "build.h"
#include "edit.tab.h"

#define INIT_MEM(array, size) if(!(array=malloc(sizeof(*(array))*(size))))exit(1);

#define TOK_NULL 0
#define TOK_REDIRECT	0x010000
#define TOK_OPERATOR	0x020000
#define TOK_RESERVED	0x040000
#define TOKEN_MASK		0x00FFFF

enum edit_shell_select_types{
	SEL_SONG=0,
	SEL_ALBUM,
	SEL_ARTIST,
	SEL_NULL
};

extern TokenList *tlist;
//extern command_t *start_command;

void editShell();

#endif

#ifndef EDIT_SHELL_H
#define EDIT_SHELL_H

#include <lex.h>
#include <build.h>
#include <y.tab.h>

#define TOK_NULL 0
#define TOK_REDIRECT	0x010000
#define TOK_OPERATOR	0x020000
#define TOK_RESERVED	0x040000
#define TOKEN_MASK		0x00FFFF

//extern TokenList *tlist;
//extern command_t *start_command;

#endif

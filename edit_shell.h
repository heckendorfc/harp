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
	SEL_PLAYLIST,
	SEL_TAG,
	SEL_NULL
};

extern TokenList *tlist;
//extern command_t *start_command;

int editShell();

#endif

/*
 *  Copyright (C) 2009-2010  Christian Heckendorf <heckendorfc@gmail.com>
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

#ifndef _DBACT_H
#define _DBACT_H

#include "defs.h"

struct insert_tps_arg{
	int order;
	int count;
	char *query;
};

unsigned int dbInit();
void dbiInit(struct dbitem *dbi);
void dbiClean(struct dbitem *dbi);
int db_exec_file(char *file);
int fetch_row(struct dbitem *dbi);
int fetch_row_at(struct dbitem *dbi, int index);
char ** fetch_column_at(struct dbitem *dbi, int index);
int doQuery(const char *querystr, struct dbitem *dbi);
int uint_return_cb(void *arg, int col_count, char **row, char **titles);
int str_return_cb(void *arg, int col_count, char **row, char **titles);
int batch_tempplaylistsong_insert_cb(void *arg, int col_count, char **row, char **titles);
int doTitleQuery(const char *querystr, int *exception, int maxwidth);
void createTempPlaylistSong();
int harp_sqlite3_exec(sqlite3*, const char *sql, int (*callback)(void*,int,char**,char**), void *arg, char **errmsg);

#endif

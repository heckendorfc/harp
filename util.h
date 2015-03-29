/*
 *  Copyright (C) 2009-2014  Christian Heckendorf <heckendorfc@gmail.com>
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

#ifndef _UTIL_H
#define _UTIL_H

#include "defs.h"

int experr(const char *epath, int eerrno);
int isURL(const char *in);
char *expand(char *in);
int fileFormat(struct pluginitem **list, const char *argv);
int getID(const char *arg);
int strToID(const char *argv);
char *getFilename(const char *path);
int *getMulti(char *arg, int *length);
int getFileTypeByName(const char *name);
int getGroupSongIDs(char *args, const int arglen, struct IDList *id_struct);

int findPluginIDByType(int type);
struct pluginitem *findPluginByID(struct pluginitem *list, int id);
int getPlugin(struct dbitem *dbi, const int index, void **module);
int getPluginModule(void **module, char *lib);
struct pluginitem **openPlugins();
struct pluginitem *closePlugin(struct pluginitem *head);
void closePluginList(struct pluginitem *head);

void cleanTempSelect(const int tempid);
int mergeTempSelect(int ida, int idb);
int insertTempSelect(const int *ids, const int idlen);
int insertTempSelectQuery(const char *query);
int insertTempSelectQueryCount(const char *query,int *count);
void db_clean(char *str, const char *data, const size_t size);
void db_safe(char *str, const char *data, const size_t size);

void miFree(struct musicInfo *mi);
void miClean(struct musicInfo *mi);

#endif

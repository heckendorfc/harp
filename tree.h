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

#ifndef _TREE_H
#define _TREE_H

struct dbnode *dbnodeAdd(struct dbnode *node);
struct dbnode *dbnodeClean(struct dbnode *node);
int *getGenreHeadPath(int head);
void printGenreHeadPath(int *path);
void printGenreChildren(struct dbnode *cur, int curid, void *action(struct dbnode*));
void tierChildPrint(struct dbnode *cur);
void tierCatPrint(struct dbnode *cur);
void printGenreTree(int head, void *action(struct dbnode *));

#endif

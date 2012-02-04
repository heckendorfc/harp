/*
 *  Copyright (C) 2009-2012  Christian Heckendorf <heckendorfc@gmail.com>
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

#ifndef _INSERT_H
#define _INSERT_H

#include "defs.h"

int getArtist(const char *arg);
int getAlbum(const char *arg, int id);
int getSong(const char *arg, const char *loc, const int id);
int getPlaylist(const char *arg);
int getCategory(const char *arg);
int getPlaylistSong(const int sid, const int pid);
int getSongCategory(const int sid, const int cid);
int batchInsert(char *arg);
int filepathInsert(struct insert_data *data);
int metadataInsert(struct insert_data *data);

#endif

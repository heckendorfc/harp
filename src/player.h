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

#ifndef _PLAYER_H
#define _PLAYER_H

#include "defs.h"

#define STATUS_REFRESH_INTERVAL (50000)
#define ADV_COM_ARG_LEN (50)
#define SAVED_PLAYLIST_NAME "Auto-Saved Playlist"

struct playercontrolarg{
	char *key;
	unsigned int cur_order;
	unsigned int next_order;
	struct termios orig;
	struct playerHandles *ph;
	struct pluginitem *decoder;
};

struct playerstatusarg{
	struct playerflag *pflag;
	struct outputdetail *outdetail;
};

int player(int list);
int getSystemKey(char key,struct playercontrolarg *pca);

#endif

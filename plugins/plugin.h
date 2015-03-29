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

#ifndef _PLUGIN_H
#define _PLUGIN_H

/*
#if WITH_ALSA==1
	#include <alsa/asoundlib.h>
#elif WITH_JACK==1
	#include <jack/jack.h>
#else
	#include <sys/soundcard.h>
	#include <sys/fcntl.h>
	#include <sys/ioctl.h>
#endif

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define MI_TITLE_SIZE 200
#define MI_TRACK_SIZE 9
#define MI_ARTIST_SIZE 100
#define MI_ALBUM_SIZE 100
#define MI_YEAR_SIZE 4

struct musicInfo{
	char *title;
	char *track;
	char *album;
	char *year;
	char *artist;
	int length;
};
*/
#include "../defs.h"
#include "../sndutil.h"

extern struct pluginitem mp3item;
extern struct pluginitem aacitem;
extern struct pluginitem vorbisitem;
extern struct pluginitem flacitem;
extern struct pluginitem streamitem;

enum plugin_ids{
	PLUGIN_MP3=0,
	PLUGIN_AAC,
	PLUGIN_VORBIS,
	PLUGIN_FLAC,
	PLUGIN_STREAM,
	PLUGIN_NULL
};

extern struct pluginitem *plugin_head[];

FILE *plugin_std_fopen(const char *path, const char *mode);
void plugin_std_fclose(FILE *fd);

#endif

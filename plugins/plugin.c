/*
 *  Copyright (C) 2009-2015  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "plugin.h"
#include "sndutil.h"

struct pluginitem *plugin_head[]={
#ifdef WITH_MP3
	&mp3item,
#else
	NULL,
#endif
#ifdef WITH_AAC
	&aacitem,
#else
	NULL,
#endif
#ifdef WITH_VORBIS
	&vorbisitem,
#else
	NULL,
#endif
#ifdef WITH_FLAC
	&flacitem,
#else
	NULL,
#endif
#ifdef WITH_STREAM
	&streamitem,
#else
	NULL,
#endif
	NULL,
};

FILE *plugin_std_fopen(const char *path, const char *mode){
	return fopen(path,mode);
}

void plugin_std_fclose(FILE *fd){
	fclose(fd);
}


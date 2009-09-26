/*
 *  Copyright (C) 2009  Christian Heckendorf <heckendorfc@gmail.com>
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


#include "sndutil.h"

#define MI_TITLE_SIZE 200
#define MI_TRACK_SIZE 9
#define MI_ARTIST_SIZE 100
#define MI_ALBUM_SIZE 100
#define MI_YEAR_SIZE 4
#define MI_LENGTH_SIZE 8

struct musicInfo{
	char *title;
	char *track;
	char *album;
	char *year;
	char *artist;
	char *length;
};

int doLocalKey(char *pubkey){
	char key=*pubkey;
	switch(key){
		case KEY_QUIT:return DEC_RET_ERROR;
		case KEY_NEXT:return DEC_RET_NEXT;
		case KEY_NEXT_NOUP:return DEC_RET_NEXT_NOUP;
		default:return DEC_RET_SUCCESS;
	}
}

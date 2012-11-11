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

#ifndef _DEFS_H
#define _DEFS_H

/*
#if WITH_ALSA==1
	#include <alsa/asoundlib.h>
#elif WITH_JACK==1
	#include <jack/jack.h>
#else
	#include <sys/fcntl.h>
	#include <sys/ioctl.h>
	#include <sys/soundcard.h>
#endif
*/

#include <ctype.h>
#include <dlfcn.h>
#include <getopt.h>
#include <glob.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <sqlite3.h>

#if WITH_ALSA==1
	#include <alsa/asoundlib.h>
#elif WITH_JACK==1
	#include <jack/jack.h>
	#include <jack/ringbuffer.h>
#elif WITH_PULSE==1
	#include <pulse/simple.h>
#else
	#include <sys/soundcard.h>
	#include <sys/fcntl.h>
	#include <sys/ioctl.h>
#endif

#define HARP_RET_ERR (-1)
#define HARP_RET_OK (0)

#define DB_PATH "~/.harp/"
#define DB "harp.db"
#define DB_BATCH_SIZE (100)

#define MI_TITLE_SIZE (200)
#define MI_TRACK_SIZE (9)
#define MI_ARTIST_SIZE (100)
#define MI_ALBUM_SIZE (100)
#define MI_YEAR_SIZE (4)
#define MI_TYPE_SIZE (30)

#define ICONF_MAX_FORMAT_SIZE (64)

#define OUTPUT_TAIL_SIZE (50)

#include "message.h"

struct dbitem{
	char **result;	  // All data
	char **row;		  // A row of data (pointers to result strings)
	int current_row;  // Row to read on the next fetch
	int row_count;    // Total data rows (header not included)
	int column_count; // Total columns
	char *err;
};

struct musicInfo{
	char *title;
	char *track;
	char *album;
	char *year;
	char *artist;
	int length;
};

extern pthread_mutex_t outstatus;
extern sqlite3 *conn;

struct playerHandles;

typedef FILE *(*function_open)(const char *path, const char *mod);
typedef void (*function_close)(FILE *ffd);
typedef int (*function_filetype)(FILE *ffd);
typedef void (*function_meta)(FILE *ffd, struct musicInfo *mi);
typedef int (*function_play)(struct playerHandles *ph, char *key, int *totaltime);
typedef void (*function_seek)(struct playerHandles *ph,int modtime);

struct pluginitem{
	void *module;
	function_open modopen;
	function_close modclose;
	function_filetype moddata;
	function_meta modmeta;
	function_play modplay;
	function_seek modseek;
	int id;
	char *contenttype;
	struct pluginitem *next;
};

struct argument{
	char arg;
	char *subarg;
	unsigned int active;
} extern arglist[];

enum cmdarg{
	ALIST,
	APLAY,
	ASHUFFLE,
	AINSERT,
	AEDIT,
	AVERBOSE,
	ATYPE,
	AZSHUFFLE,
	AADMIN,
	ADEVICE,
	AREPEAT
};

enum filetype{
	FMP3=1,
	FASF,
	FM4A,
	FVOR,
	FFLAC
};

enum portal_ret{
	PORTAL_RET_MAIN=-1,
	PORTAL_RET_QUIT=0,
	PORTAL_RET_PREV=1
};

struct insert_data{
	int fmt;
	char *query;
	struct musicInfo *mi;
	struct dbitem *dbi;
	const char *path;
	struct pluginitem *plugin_head;
};

struct lconf{
	int maxwidth;
	int exception;
}extern listconf;

struct iconf{
	char **f_root;
	char **format;
	int format_length;
	int (*first_cb)(struct insert_data *data);
	int (*second_cb)(struct insert_data *data);
	int length;
}extern insertconf;

struct dconf{
	char *dir;
	int log;
	FILE *playfd;
	char *playfilename;
	FILE *msgfd;
	char *msgfilename;
	int level;
}extern debugconf;

struct commandOption{
	char opt;
	int (*function)(char *args, void *data);
	const char *help;
	void *data;
};

struct dbnode{
	struct dbitem dbi;
	struct dbnode *prev;
	struct dbnode *next;
	int depth;
};

struct titlequery_data{
	int count;
	int maxwidth;
	int *exception;
	int *exlen;
};

struct IDList {
	int *songid;
	int length;
	int tempselectid;
};

/* */

struct playerflag{
	volatile int pause;
	int mute;
	volatile int update;
	volatile int rating;
	volatile int exit;
	volatile char mutec;
	volatile char pausec;
};

enum defkeys{
	KEY_QUIT='q',
	KEY_NEXT='n',
	KEY_NEXT_NOUP='N',
	KEY_PREV='p',
	KEY_VOLUP='0',
	KEY_VOLDN='9',
	KEY_MUTE='m',
	KEY_PAUSE=' ',
	KEY_RATEUP='R',
	KEY_RATEDN='r',
	KEY_SEEK_UP='.',
	KEY_SEEK_DN=',',
	KEY_COMMAND=':',
	KEY_NULL=0
};

enum decreturns{
	DEC_RET_ERROR=1,
	DEC_RET_SUCCESS,
	DEC_RET_NEXT,
	DEC_RET_NEXT_NOUP
};

struct outputdetail{
	int curtime;
	int totaltime;
	int percent;
	int status;
	char tail[OUTPUT_TAIL_SIZE];
};

struct playerHandles{
	FILE *ffd;
	char *device;
#if WITH_ALSA==1
	snd_pcm_t *sndfd;
	snd_pcm_hw_params_t *params;
#elif WITH_JACK==1
	jack_client_t *sndfd;
	jack_port_t *out_port1, *out_port2;
	const char **jack_ports;
	float vol_mod;
	jack_default_audio_sample_t *tmpbuf;
	jack_ringbuffer_t **outbuf;
	int maxsize;
	int out_gain;
#elif WITH_PULSE==1
	pa_simple *sndfd;
	pa_sample_spec ss;
	//pa_context *sndfd;
	//pa_stream *stream;
	//pa_threaded_mainloop *mainloop;
#else
	int sndfd;
	int *params;
#endif
	int dec_enc,dec_chan,dec_rate,out_rate;
	struct playerflag *pflag;
	void *dechandle;
	struct outputdetail *outdetail;
	struct pluginitem *plugin_head;
};

/* */

#endif

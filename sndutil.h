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
	#include <jack/ringbuffer.h>
#else
	#include <sys/soundcard.h>
	#include <sys/fcntl.h>
	#include <sys/ioctl.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct playerflag{
	int pause;
	int mute;
	int update;
	int rating;
	char mutec;
	char pausec;
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
	DEC_RET_ERROR,
	DEC_RET_SUCCESS,
	DEC_RET_NEXT,
	DEC_RET_NEXT_NOUP
};

struct outputdetail{
	int curtime;
	int totaltime;
	int percent;
	int status;
};

struct playerHandles{
	FILE *ffd;
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
#else
	int sndfd;
	int *params;
#endif
	int dec_enc,dec_chan,dec_rate,out_rate;
	struct playerflag *pflag;
	void *dechandle;
	struct outputdetail *outdetail;
};


//sndutil.c
int snd_init(struct playerHandles *ph);
int snd_param_init(struct playerHandles *ph, int *enc, int *channels, unsigned int *rate);
void toggleMute(struct playerHandles *ph, int *mute);
void changeVolume(struct playerHandles *ph, int mod);
void snd_clear(struct playerHandles *ph);
int writei_snd(struct playerHandles *ph, const char *out, const unsigned int size);
int writen_snd(struct playerHandles *ph, void *out[], const unsigned int size);
void snd_close(struct playerHandles *ph);

#if WITH_ALSA==1
	#include "alsautil.c"
#elif WITH_JACK==1
	#include "jackutil.c"
#else
	#include "ossutil.c"
#endif

void crOutput(struct playerflag *pflag, struct outputdetail *details){
	if(pflag->update){
		fprintf(stdout,"\r [%c %c][%ds of %ds (%d%%)]", pflag->pausec, pflag->mutec, details->curtime, details->totaltime, details->percent<-1?-1:details->percent);
		fflush(stdout);
	}
}


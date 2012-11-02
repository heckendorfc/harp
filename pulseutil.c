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

#include "alsautil.h"
#include "sndutil.h"

int snd_init(struct playerHandles *ph){
	int error;

	ph->ss.format=PA_SAMPLE_S16LE;
	ph->ss.channels=2;
	ph->ss.rate=44100;

	ph->sndfd=pa_simple_new(NULL, "HARP", PA_STREAM_PLAYBACK, NULL, "music", &ph->ss, NULL, NULL, &error);
	if(!ph->sndfd){
		fprintf(stderr,"sndfd open failed: %d\n",error);
		return 1;
	}

	return 0;
}

int snd_param_init(struct playerHandles *ph, int *enc, int *channels, unsigned int *rate){
	int error;
    struct pa_sample_spec ss;

	if(ph->ss.channels==*channels && ph->ss.rate==*rate) // Already configured properly
		return 0;

    ph->ss.channels = *channels;
    ph->ss.rate = *rate;

	if(ph->sndfd)
		pa_simple_free(ph->sndfd);

	if(pa_simple_drain(ph->sndfd,&error)<0){
		fprintf(stderr,"drain failed: %d\n",error);
		return 1;
	}
	ph->sndfd=pa_simple_new(NULL, "HARP", PA_STREAM_PLAYBACK, NULL, "music", &ph->ss, NULL, NULL, &error);
	if(!ph->sndfd){
		fprintf(stderr,"sndfd open failed: %d\n",error);
		return 1;
	}

	return 0;
}

void changeVolume(struct playerHandles *ph, int mod){
}

void toggleMute(struct playerHandles *ph, int *mute){
}

void snd_clear(struct playerHandles *ph){
	int error;
	if(pa_simple_flush(ph->sndfd,&error)<0)
		fprintf(stderr,"Failed to flush audio\n");
}

int writei_snd(struct playerHandles *ph, const char *out, const unsigned int size){
	int ret;
	int error;
	if(ph->pflag->pause){
		snd_clear(ph);
		do{
			usleep(100000); // 0.1 seconds
		}
		while(ph->pflag->pause);
	}
	ret=pa_simple_write(ph->sndfd,out,(size_t)size,&error);
	if(ret<0)
		fprintf(stderr,"Failed to write audio: %d\n",error);
	return 0;
}

void snd_close(struct playerHandles *ph){
	pa_simple_free(ph->sndfd);
}

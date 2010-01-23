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

#ifndef _SNDUTIL_H
#define _SNDUTIL_H


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"

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

#endif

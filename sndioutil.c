/*
 *  Copyright (C) 2016  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "sndioutil.h"
#include "sndutil.h"

int snd_init(struct playerHandles *ph){
	const int nonblocking_io=0;
	ph->sndio_started=0;
	if((ph->sndfd=sio_open(ph->device?ph->device:SIO_DEVANY,SIO_PLAY,nonblocking_io))==NULL)
		return 1;
	return 0;
}

int snd_param_init(struct playerHandles *ph, int *enc, int *channels, unsigned int *rate){
	struct sio_par par;

	if(ph->sndio_started){
		sio_stop(ph->sndfd);
		ph->sndio_started=0;
	}
	sio_initpar(&par);

	par.bits=*enc;
	par.pchan=*channels;
	par.rate=*rate;

	if(sio_setpar(ph->sndfd,&par)==0){
		fprintf(stderr,"sndio param error\n");
		return 1;
	}

	if(!ph->sndio_started){
		sio_start(ph->sndfd);
		ph->sndio_started=1;
	}

	return 0;
}

void changeVolume(struct playerHandles *ph, int mod){
	//sio_setvol
}

void toggleMute(struct playerHandles *ph, int *mute){
	//sio_setvol
}

void snd_clear(struct playerHandles *ph){
}

int writei_snd(struct playerHandles *ph, const char *out, const unsigned int size){
	size_t write_size;

	if(ph->pflag->pause){
		do{
			usleep(100000);
		}
		while(ph->pflag->pause);
	}

	if((write_size=sio_write(ph->sndfd,out,size))!=size)
		fprintf(stderr,"Write error %d %d\n",size,write_size);

	return write_size;
}

void snd_close(struct playerHandles *ph){
	sio_close(ph->sndfd);
}


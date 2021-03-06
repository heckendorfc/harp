/*
 *  Copyright (C) 2009-2014  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "ossutil.h"
#include "sndutil.h"

int snd_init(struct playerHandles *ph){
	if((ph->sndfd=open(ph->device?ph->device:"/dev/dsp",O_WRONLY,777))==-1)return 1;
	return 0;
}

int snd_param_init(struct playerHandles *ph, int *enc, int *channels, unsigned int *rate){
	int oss_enc=AFMT_S16_NE;
	if(ioctl(ph->sndfd,SNDCTL_DSP_RESET,NULL)<0){
		fprintf(stderr,"reset errno:%d\n",errno);
		errno=0;
	}
	if(ioctl(ph->sndfd,SNDCTL_DSP_SETFMT,&oss_enc)<0){
		fprintf(stderr,"fmt errno:%d\n",errno);
		errno=0;
	}
	if(ioctl(ph->sndfd,SNDCTL_DSP_CHANNELS,channels)<0){
		fprintf(stderr,"ch errno:%d\n",errno);
		errno=0;
	}
	if(ioctl(ph->sndfd,SNDCTL_DSP_SPEED,rate)<0){
		fprintf(stderr,"rate errno:%d\n",errno);
		errno=0;
	}
	fprintf(stderr,"param ok");
	return 0;
}

void changeVolume(struct playerHandles *ph, int mod){
#ifdef OSSV4_DEFS
	int current;
	int ffd=ph->sndfd;
	char tail[OUTPUT_TAIL_SIZE];

	if(ph->pflag->mute)
		current=ph->pflag->mute;
	else
		if(ioctl(ffd,SNDCTL_DSP_GETPLAYVOL,&current)==-1){fprintf(stderr,"\nget vol errno:%d\n",errno);errno=0;close(ffd);return;}

	current+=mod<<8;
	current+=mod;
	if((current&0xff)>150)current=0; // Unsigned integer overloaded through 0
	if((current&0xff)>100)current=100+(100<<8);
	if(ph->pflag->mute){
		if(!(ph->pflag->mute=current))
			ph->pflag->mutec=32;
	}
	else
		if(ioctl(ffd,SNDCTL_DSP_SETPLAYVOL,&current)==-1){fprintf(stderr,"\nset vol errno:%d\n",errno);errno=0;close(ffd);return;}

	sprintf(tail,"Volume: %d%%",(0xff&current));
	addStatusTail(tail,ph->outdetail);
#endif
}

void toggleMute(struct playerHandles *ph, int *mute){
#ifdef OSSV4_DEFS
	int current;
	int ffd=ph->sndfd;

	if(*mute>0){ // Unmute and perform volume change
		char tail[OUTPUT_TAIL_SIZE];
		current=*mute;
		*mute=0;
		sprintf(tail,"Volume: %d%%",(0xff&current));
		addStatusTail(tail,ph->outdetail);
	}
	else{ // Mute
		if(ioctl(ffd,SNDCTL_DSP_GETPLAYVOL,&current)==-1){fprintf(stderr,"\nget vol errno:%d\n",errno);errno=0;close(ffd);return;}
		*mute=current;
		current=0;
		addStatusTail("Volume Muted",ph->outdetail);
	}
	fflush(stdout);

	if(ioctl(ffd,SNDCTL_DSP_SETPLAYVOL,&current)==-1){fprintf(stderr,"\nset vol errno:%d\n",errno);errno=0;close(ffd);return;}
#endif
}

void snd_clear(struct playerHandles *ph){
#ifdef OSSV4_DEFS
	ioctl(ph->sndfd,SNDCTL_DSP_SKIP,NULL);
#endif
}

int writei_snd(struct playerHandles *ph, const char *out, const unsigned int size){
	int write_size;
	if(ph->pflag->pause){
#ifdef OSSV4_DEFS
		ioctl(ph->sndfd,SNDCTL_DSP_SILENCE,NULL);
#endif
		do{
			usleep(100000);
		}
		while(ph->pflag->pause);
#ifdef OSSV4_DEFS
		ioctl(ph->sndfd,SNDCTL_DSP_SKIP,NULL);
#endif
	}
	if(size==0){
		return 0;
	}
	if((write_size=write(ph->sndfd,out,size))!=size)
		fprintf(stderr,"Write error %d %d\n",size,write_size);
	return write_size;
}

void snd_close(struct playerHandles *ph){
	close(ph->sndfd);
}

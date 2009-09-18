/*
 *  Copyright (C) 2009  Christian Heckendorf <vaseros@gmail.com>
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

#include <errno.h>

int snd_init(struct playerHandles *ph){
	if((ph->sndfd=open("/dev/dsp",O_WRONLY,777))==-1)return 1;
	fprintf(stderr,"open ok");
	return 0;
}

int snd_param_init(struct playerHandles *ph, int *enc, int *channels, unsigned int *rate){
	*enc=AFMT_S16_NE;
	if(ioctl(ph->sndfd,SNDCTL_DSP_RESET,NULL)<0){
		fprintf(stderr,"reset errno:%d\n",errno);
		errno=0;
	}
	if(ioctl(ph->sndfd,SNDCTL_DSP_SETFMT,enc)<0){
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
	int current;
	int ffd=ph->sndfd;
	//if((ffd=open("/dev/dsp",O_RDWR,777))<0)return;

	if(ioctl(ffd,SNDCTL_DSP_GETPLAYVOL,&current)==-1){fprintf(stderr,"\nget vol errno:%d\n",errno);errno=0;close(ffd);return;}

	current+=mod<<8;
	current+=mod;
	if((current&0xff)>0)
		if(ioctl(ffd,SNDCTL_DSP_SETPLAYVOL,&current)==-1){fprintf(stderr,"\nset vol errno:%d\n",errno);errno=0;close(ffd);return;}

	fprintf(stdout,"\r                               Volume: %d%%  ",(0xff&current));
	fflush(stdout);

	//close(ffd);
}

void toggleMute(struct playerHandles *ph, int *mute){
	int current;
	int ffd=ph->sndfd;
	//if((ffd=open("/dev/dsp",O_RDWR,777))<0)return;

	if(*mute>0){ // Unmute and perform volume change
		current=*mute;
		*mute=0;
		fprintf(stdout,"\r                               Volume: %d%%  ",(0xff&current));
	}
	else{ // Mute 
		if(ioctl(ffd,SNDCTL_DSP_GETPLAYVOL,&current)==-1){fprintf(stderr,"\nget vol errno:%d\n",errno);errno=0;close(ffd);return;}
		*mute=current;
		current=0;
		fprintf(stdout,"\r                               Volume Muted  ");
	}
	fflush(stdout);

	if(ioctl(ffd,SNDCTL_DSP_SETPLAYVOL,&current)==-1){fprintf(stderr,"\nset vol errno:%d\n",errno);errno=0;close(ffd);return;}

	//close(ffd);
}

void snd_clear(struct playerHandles *ph){
	ioctl(ph->sndfd,SNDCTL_DSP_SKIP,NULL);
}

int writei_snd(struct playerHandles *ph, const char *out, const unsigned int size){
	int write_size;
	if(ph->pflag->pause){
		ioctl(ph->sndfd,SNDCTL_DSP_SILENCE,NULL);
		do{
			usleep(100000);
		}
		while(ph->pflag->pause);
		ioctl(ph->sndfd,SNDCTL_DSP_SKIP,NULL);
	}
	if((write_size=write(ph->sndfd,out,size))!=size)
		fprintf(stderr,"Write error %d %d\n",size,write_size);
	return write_size;
}

int writen_snd(struct playerHandles *ph, void *out[], const unsigned int size){
	return 0;
}

void snd_close(struct playerHandles *ph){
	close(ph->sndfd);
}

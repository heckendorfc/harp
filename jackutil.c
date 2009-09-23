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

/* struct vars:

jack_client_t *sndfd;
jack_port_t *in_port, *out_port;
const char **jack_ports;
char *outbuf,*startptr;
int fillsize; // Empty space in the buffer
int maxsize; // Total buffer capacity

*/

pthread_mutex_t outbuf_lock;

#include <math.h>

#define NORMFACT (float)0x8000
#define GAIN_MAX 10
#define GAIN_MIN -40

int decimate(float *buf, int len, int M){
	int x;
	for(x=0;x<len;x++){
		buf[x]=buf[x*M];
	}
	return len/M;
}

int interpolate(float *buf, int len, int L){
	int x,y;
	for(x=len;x>0;x--){
		buf[x*L]=buf[x];
		for(y=0;y<L;y++)
			buf[y+x]=0;
	}
	// TODO" Add FIR
	return len*L;
}

int resample(float *buf, int len, int maxlen, int in_rate, int out_rate){
	int x,y,L,M,newlen;
	if(in_rate==out_rate)return len;
	if(in_rate<out_rate){
		if(!(in_rate%out_rate)){ // Resampling not supported yet
			return len;
		}

		L=out_rate/in_rate;
		if(len*L>maxlen){ // Buffer overflow
			return -(len*L);
		}
		newlen=interpolate(buf,len,L);
	}
	else{
		if(!(out_rate%in_rate)){ // Resampling not supported yet
			return len;
		}

		M=in_rate/out_rate;
		newlen=decimate(buf,len,M);
	}

	return newlen;
}

int char_to_float(float *f, const char *c, int len){
	int i;

	if(len%2){
		return -1;
	}

	short *in = (short*)c;
	len>>=1;
	for (i=0; i<len; i++) {
		f[i] = in[i]/NORMFACT;
	}
	return len;
}

void jack_err(const char *err){
	fprintf(stderr,"JACK ERR: %s\n",err);
}

int jack_process(jack_nframes_t nframes, void *arg){
	struct playerHandles *ph = (struct playerHandles*)arg;
	const float vol_mod=ph->vol_mod;
	int x;

	if(!ph->dechandle)return 0;

	pthread_mutex_lock(&outbuf_lock);
		jack_nframes_t combined_nframes=nframes<<(ph->dec_chan/2);
		jack_nframes_t actual=(combined_nframes>ph->fillsize)?(ph->fillsize):combined_nframes;

		jack_default_audio_sample_t *out1=(jack_default_audio_sample_t *)jack_port_get_buffer(ph->out_port1,nframes);
		jack_default_audio_sample_t *out2=(jack_default_audio_sample_t *)jack_port_get_buffer(ph->out_port2,nframes);

		if(!ph->pflag->mute){
			if(ph->dec_chan==1){
				for(x=0;x<actual;x++)
					out1[x]=out2[x]=ph->outbuf[x]*vol_mod;
			}
			else{
				for(x=0;x<actual;x++){
					out1[x]=ph->outbuf[(x<<1)]*vol_mod;
					out2[x]=ph->outbuf[(x<<1)+1]*vol_mod;
				}
			}
		}
		else // Fill with silence
			x=0;

		// Silence any remaining audio
		for(;x<nframes;x++)
			out1[x]=out2[x]=0;

		// Move remaining data to the front.
		memmove(ph->outbuf,ph->outbuf+actual,sizeof(float)*(ph->fillsize-actual));

		ph->fillsize-=actual;
	pthread_mutex_unlock(&outbuf_lock);

	return 0;
}

int jack_srate(jack_nframes_t nframes, void *arg){
	struct playerHandles *ph = (struct playerHandles*)arg;
	ph->out_rate=nframes;
	fprintf(stderr,"JACK | set rate: %d\n",nframes);
	return 0;
}

void jack_shutdown(void *arg){
	fprintf(stderr,"JACK | Shutdown.\n");
}

int snd_init(struct playerHandles *ph){
	char client_name[255];
	ph->maxsize=40960;
	ph->fillsize=0;
	ph->outbuf=calloc((ph->maxsize),sizeof(float));
	pthread_mutex_init(&outbuf_lock,NULL);

	ph->out_gain=0;
	ph->vol_mod=1;
	jack_set_error_function(jack_err);

	snprintf(client_name,255,"HARP-%d",getpid());
	if((ph->sndfd=jack_client_open(client_name,JackNullOption,NULL))==NULL){
		fprintf(stderr,"JACK | Jack server not running?\n");
		return 1;
	}

	jack_set_process_callback(ph->sndfd,jack_process,ph);
	jack_set_sample_rate_callback(ph->sndfd,jack_srate,ph);
	jack_on_shutdown(ph->sndfd,jack_shutdown,ph);

	if((ph->out_port1=jack_port_register(ph->sndfd,"output1",JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0))==NULL){
		fprintf(stderr,"JACK | Can't register output port.\n");
		return 1;
	}
	if((ph->out_port2=jack_port_register(ph->sndfd,"output2",JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0))==NULL){
		fprintf(stderr,"JACK | Can't register output port.\n");
		return 1;
	}

	if(jack_activate(ph->sndfd)){
		fprintf(stderr,"JACK | Can't activate.\n");
		return 1;
	}

	if((ph->jack_ports=jack_get_ports(ph->sndfd,NULL,NULL,JackPortIsPhysical|JackPortIsInput))==NULL){
		fprintf(stderr,"JACK | Can't find any physial output ports.\n");
		return 1;
	}

	if(jack_connect(ph->sndfd, jack_port_name(ph->out_port1), ph->jack_ports[0])){
		fprintf (stderr, "JACK | Can't connect output ports 0\n");
	}
	if(jack_connect(ph->sndfd, jack_port_name(ph->out_port2), ph->jack_ports[1])){
		fprintf (stderr, "JACK | Can't connect output ports 0\n");
	}

	return 0;
}

int snd_param_init(struct playerHandles *ph, int *enc, int *channels, unsigned int *rate){
	int x=0;
	ph->dec_rate=*rate;
	ph->dec_chan=*channels;
	ph->dec_enc=*enc;
	return 0;
}

void changeVolume(struct playerHandles *ph, int mod){
	int current;
	mod=mod>0?2:-2;
	ph->out_gain+=mod; // 5 DB increment is too much
	if(ph->out_gain>GAIN_MAX)ph->out_gain=GAIN_MAX;
	else if(ph->out_gain<GAIN_MIN)ph->out_gain=GAIN_MIN;

	ph->vol_mod=powf(10.0f,(float)ph->out_gain*0.05f);

	fprintf(stdout,"\r                               Volume: %d%% (%dDb)      ",(int)((ph->out_gain-GAIN_MIN)*2),ph->out_gain);
	fflush(stdout);
}

void toggleMute(struct playerHandles *ph, int *mute){
	int x;

	if(*mute>0){ // Unmute and perform volume change
		*mute=0;
		fprintf(stdout,"\r                               Volume: %d%% (%dDb)      ",(int)((ph->out_gain-GAIN_MIN)*2),ph->out_gain);
	}
	else{ // Mute 
		*mute=1;
		fprintf(stdout,"\r                               Volume Muted           ");
	}
	fflush(stdout);
}

void snd_clear(struct playerHandles *ph){
	ph->fillsize=0;
}

int writei_snd(struct playerHandles *ph, const char *out, const unsigned int size){
	int ret,resamp,remaining=size;
	if(ph->pflag->pause){ // Move this into process?
		ret=ph->fillsize; // Can't let all that beautiful music go to waste...
		snd_clear(ph);
		do{
			usleep(100000); // 0.1 seconds
		}
		while(ph->pflag->pause);
		ph->fillsize=ret;
	}
	do{
		if(ph->maxsize-ph->fillsize>remaining){
			pthread_mutex_lock(&outbuf_lock);
				if((ret=char_to_float(ph->outbuf+ph->fillsize,out,remaining))>=0){
					if((resamp=resample(ph->outbuf+ph->fillsize,ret,ph->maxsize-ph->fillsize,ph->dec_rate,ph->out_rate))<0){
						pthread_mutex_unlock(&outbuf_lock);
						usleep(12000);
						continue;
					}
					ph->fillsize+=resamp;
				}
				else{
					fprintf(stderr,"JACK | Err: Float conversion failed.");
				}
				remaining=ret=0;
			pthread_mutex_unlock(&outbuf_lock);
		}
		usleep(6000); // 0.006 seconds
	}while(remaining>0);
	return 0;
}

int writen_snd(struct playerHandles *ph, void *out[], const unsigned int size){
}

void snd_close(struct playerHandles *ph){
	jack_client_close(ph->sndfd);
	free(ph->outbuf);
	free(ph->jack_ports);
}

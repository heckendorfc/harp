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

#include "jackutil.h"
#include "sndutil.h"

pthread_mutex_t outbuf_lock;

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

int resample(jack_default_audio_sample_t *buf, int len, int maxlen, int in_rate, int out_rate){
	int x,y,L,M,newlen;
	if(in_rate==out_rate)return len;
	fprintf(stderr,"\nResampling\n");
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
	if(!ph->dechandle) return 0;

	size_t actual = sizeof(jack_default_audio_sample_t)*nframes;
	size_t read;
	char *out1=(char *)jack_port_get_buffer(ph->out_port1,nframes);
	char *out2=(char *)jack_port_get_buffer(ph->out_port2,nframes);

	if(!ph->pflag->mute){
		if(ph->dec_chan==1){
			read=jack_ringbuffer_read(ph->outbuf[0],out1,actual);
			if(read<actual){
				bzero(out1+read,actual-read);
			}
			memcpy(out2,out1,actual);
		}
		else{
			read=jack_ringbuffer_read(ph->outbuf[0],out1,actual);
			if(read<actual)
				bzero(out1+read,actual-read);
			read=jack_ringbuffer_read(ph->outbuf[1],out2,actual);
			if(read<actual)
				bzero(out2+read,actual-read);
		}
	}
	else{ // Fill with silence
		bzero(out1,actual);
		bzero(out2,actual);
	}

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
	//ph->pflag->update=0;

	char client_name[255];
	ph->maxsize=40960;
	if(!(ph->outbuf=malloc(2*sizeof(*ph->outbuf)))
		|| !(ph->tmpbuf=malloc(sizeof(*ph->tmpbuf)))){
		fprintf(stderr,"Malloc failed (jack buf).");
		return 1;
	}
	
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

	ph->maxsize=jack_get_sample_rate(ph->sndfd);
	fprintf(stderr,"Buffer: %d (x2)\n",ph->maxsize);
	ph->outbuf[0]=jack_ringbuffer_create(ph->maxsize);
	ph->outbuf[1]=jack_ringbuffer_create(ph->maxsize);

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
	char tail[OUTPUT_TAIL_SIZE];
	mod=mod>0?2:-2;
	ph->out_gain+=mod; // 5 DB increment is too much
	if(ph->out_gain>GAIN_MAX)ph->out_gain=GAIN_MAX;
	else if(ph->out_gain<GAIN_MIN)ph->out_gain=GAIN_MIN;

	ph->vol_mod=powf(10.0f,(float)ph->out_gain*0.05f);

	sprintf(tail,"Volume: %d%% (%dDb)",(int)((ph->out_gain-GAIN_MIN)*2),ph->out_gain);
	addStatusTail(tail,ph->outdetail);
}

void toggleMute(struct playerHandles *ph, int *mute){
	int x;

	if(*mute>0){ // Unmute and perform volume change
		char tail[OUTPUT_TAIL_SIZE];
		*mute=0;
		sprintf(tail,"Volume: %d%% (%dDb)",(int)((ph->out_gain-GAIN_MIN)*2),ph->out_gain);
		addStatusTail(tail,ph->outdetail);
	}
	else{ // Mute 
		*mute=1;
		addStatusTail("Volume Muted",ph->outdetail);
	}
	fflush(stdout);
}

void snd_clear(struct playerHandles *ph){
	jack_ringbuffer_reset(ph->outbuf[0]);
	jack_ringbuffer_reset(ph->outbuf[1]);
}

int buf_convert(jack_default_audio_sample_t *out, const char *in, int len, const float mod){
	int i;

	if(len%2){
		return -1;
	}

	short *s_in = (short*)in;
	len>>=1;
	for (i=0; i<len; i++) {
		out[i] = (s_in[i]/NORMFACT)*mod;
	}
	return len;
}

int writei_snd(struct playerHandles *ph, const char *out, const unsigned int size){
	int c,i,ret,tmpbufsize;
	const int chan_shift=ph->dec_chan>>1;
	const int chan_size=size>>chan_shift;
	const int samples=chan_size>>1; /* for short -> float conversion */

	if(ph->pflag->pause){ // Move this into process?
		size_t write[2],read[2];
		read[0]=ph->outbuf[0]->read_ptr;
		read[1]=ph->outbuf[1]->read_ptr;
		write[0]=ph->outbuf[0]->write_ptr;
		write[1]=ph->outbuf[1]->write_ptr;
		// Can't let all that beautiful music go to waste...
		snd_clear(ph);
		do{
			usleep(100000); // 0.1 seconds
		}
		while(ph->pflag->pause);
		ph->outbuf[0]->read_ptr=read[0];
		ph->outbuf[1]->read_ptr=read[1];
		ph->outbuf[0]->write_ptr=write[0];
		ph->outbuf[1]->write_ptr=write[1];
	}

	tmpbufsize=samples*(ph->dec_rate/ph->out_rate);
	if(!(ph->tmpbuf=realloc(ph->tmpbuf,tmpbufsize*sizeof(*ph->tmpbuf)))){
		fprintf(stderr,"JACK | temp buffer failed reallocation\n");
		return 0;
	}

	i=tmpbufsize*sizeof(*ph->tmpbuf);
	while(jack_ringbuffer_write_space(ph->outbuf[0])<i){
		usleep(100000);
	}

	short *s_in = (short*)out;
	for(c=0;c<ph->dec_chan;c++){
		for (i=0; i<samples; i++)
			ph->tmpbuf[i] = (s_in[(i<<chan_shift)+c]/NORMFACT)*ph->vol_mod;
		if((ret=resample(ph->tmpbuf, samples, tmpbufsize, ph->dec_rate, ph->out_rate))!=tmpbufsize)
			fprintf(stderr,"\nResample size mismatch: ret %d | buf %d\n", ret, tmpbufsize);
		i=tmpbufsize*sizeof(*ph->tmpbuf);
		if((ret=jack_ringbuffer_write(ph->outbuf[c],(char*)ph->tmpbuf,i))<i)
			fprintf(stderr,"JACK | ringbuffer failed write. expected: %d ; got: %d\n",i,ret);
	}

	return 0;
}

void snd_close(struct playerHandles *ph){
	jack_client_close(ph->sndfd);
	jack_ringbuffer_free(ph->outbuf[0]);
	jack_ringbuffer_free(ph->outbuf[1]);
	free(ph->outbuf);
	free(ph->jack_ports);
}

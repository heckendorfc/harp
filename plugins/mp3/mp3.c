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

#include "plugin.h"
#include <mpg123.h>
#include "mp3meta.c"

pthread_mutex_t dechandle_lock;

struct mp3Handles{
	mpg123_handle *m;
	long total;
	int accuracy;
	int framesize;
	int samptime;
}h;

FILE * plugin_open(const char *path, const char *mode){
	return plugin_std_fopen(path,mode);
}

void plugin_close(FILE *ffd){
	plugin_std_fclose(ffd);
}

int filetype_by_data(FILE *ffd){
	unsigned char buf[10];
	fseek(ffd,0,SEEK_SET);
	if(fread(buf,sizeof(buf),1,ffd)<1)return 0;
	if(buf[0]=='I' && buf[1]=='D' && buf[2]=='3'){
		return 1;
	}
	fseek(ffd,-(128*sizeof(buf[0])),SEEK_END);
	if(fread(buf,sizeof(buf),1,ffd)<1)return 0;
	if(buf[0]=='T' && buf[1]=='A' && buf[2]=='G'){
		return 1;
	}
	return 0;
}

void new_format(struct playerHandles *ph){
	long ratel;
	int channels, enc,enc_bit=2;
	unsigned int rate;
	
	mpg123_getformat(h.m, &ratel, &channels, &enc);
	rate=(unsigned int)ratel;
	fprintf(stderr,"New format: %dHz %d channels %d encoding\n",(int)ratel, channels, enc_bit*8);
	snd_param_init(ph,&enc_bit,&channels,&rate);

	ph->dec_rate=ratel;
	ph->dec_enc=enc_bit;
	ph->dec_chan=channels;
	h.framesize=enc_bit*channels;
	h.samptime=ratel*h.framesize;
}

int mp3Init(struct playerHandles *ph){
	pthread_mutex_init(&dechandle_lock,NULL);

	mpg123_init();

	h.m = mpg123_new(NULL, NULL);
	if(h.m == NULL){
		fprintf(stderr,"Unable to create mpg123 handle\n");
		return -1;
	}
	//mpg123_param(h.m, MPG123_VERBOSE, 0, 0); 
	mpg123_param(h.m, MPG123_RESYNC_LIMIT, -1, 0); 
	mpg123_param(h.m, MPG123_FLAGS, MPG123_QUIET, 0); 
	//mpg123_open_feed(h.m);
	mpg123_open_fd(h.m,fileno(ph->ffd));
	if(h.m == NULL) return -1;
	
	return 0;
}

void plugin_seek(struct playerHandles *ph, int modtime){
	if(ph->dechandle==NULL){
		fprintf(stderr,"no dechandle");
		return;
	}

pthread_mutex_lock(&dechandle_lock);
	struct mp3Handles *h=(struct mp3Handles *)ph->dechandle;
	mpg123_seek_frame(h->m,mpg123_timeframe(h->m,(int)modtime),modtime?SEEK_CUR:SEEK_SET);


	if(modtime){
		h->total+=(modtime*h->accuracy);
		if(h->total<0)h->total=0;
	}
	else
		h->total=0;
pthread_mutex_unlock(&dechandle_lock);

	snd_clear(ph);
}

void plugin_exit(struct playerHandles *ph){
	if(h.m!=NULL)mpg123_delete(h.m);
	mpg123_exit();
	pthread_mutex_destroy(&dechandle_lock);
}

int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	size_t size;
	size_t len;
	int mret=MPG123_NEED_MORE;
	int retval=DEC_RET_SUCCESS;
	struct outputdetail details;
	long ratel;
	int channels, enc,enc_bit=2;
	unsigned int rate;
	unsigned char *out;
	int outsize;
	
	if(mp3Init(ph)<0)return DEC_RET_ERROR;

	details.totaltime=*totaltime>0?*totaltime:-1;
	details.percent=-1;
	ph->dechandle=&h;
			
	pthread_mutex_lock(&dechandle_lock);
		new_format(ph);
		outsize=mpg123_outblock(h.m);
	pthread_mutex_unlock(&dechandle_lock);

	h.total=0;
	h.accuracy=1000;

	if(!(out=malloc(sizeof(unsigned char)*outsize))){
		fprintf(stderr,"Malloc failed (out decoder buffer).");
		plugin_exit(ph);
		return DEC_RET_ERROR;
	}

	do{ /* Read and write until everything is through. */

		//if outbuff isn't big enough to hold all decoded data from inbuff, keep going
		if(mret != MPG123_NEED_MORE){
			pthread_mutex_lock(&dechandle_lock);
				mret=mpg123_read(h.m,out,outsize,&len);
			pthread_mutex_unlock(&dechandle_lock);
		}
		else{
			pthread_mutex_lock(&dechandle_lock);
				mret=mpg123_read(h.m,out,outsize,&len);
			pthread_mutex_unlock(&dechandle_lock);
			if(mret==MPG123_DONE || mret==MPG123_ERR){ // EOF (or read error)
				retval=DEC_RET_SUCCESS;
				fprintf(stderr,"done..");
				break;
			}
		}
		if(mret==MPG123_NEW_FORMAT){
			//fprintf(stderr,"Should have reformatted here.");
			pthread_mutex_lock(&dechandle_lock);
				new_format(ph);
			pthread_mutex_unlock(&dechandle_lock);
		}
		if(len==0)continue;
		size=len;

		pthread_mutex_lock(&dechandle_lock);
		details.curtime=h.total/h.accuracy;
		details.percent=(details.curtime*100)/details.totaltime;
		crOutput(ph->pflag,&details);
		h.total+=(size*h.accuracy)/h.samptime;
		pthread_mutex_unlock(&dechandle_lock);

#if WITH_ALSA==1
		if(writei_snd(ph,(char *)out,size/h.framesize)<0)break;
#else
		if(writei_snd(ph,(char *)out,size)<0)break;
#endif

		if(ph->pflag->exit!=DEC_RET_SUCCESS){
			retval=ph->pflag->exit;
			break;	
		}
	}while(mret != MPG123_ERR && mret!=MPG123_DONE);
	if(mret == MPG123_ERR){
		fprintf(stderr, "decoder error: %s", mpg123_strerror(h.m)); 
		if(mpg123_errcode(h.m)!=28) // Common error. quick fix
			retval=DEC_RET_ERROR;
	}

	/* Done decoding, now just clean up and leave. */
	plugin_exit(ph);
	free(out);
	*totaltime=details.curtime;
	return retval;
}

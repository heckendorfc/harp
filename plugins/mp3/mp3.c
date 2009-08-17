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

#include "plugin.h"
#include <mpg123.h>
#include "mp3meta.c"

struct mp3Handles{
	mpg123_handle *m;
	long *total;
	int accuracy;
}h;

int filetype_by_data(FILE *ffd){
	unsigned char buf[10];
	fseek(ffd,0,SEEK_SET);
	fread(buf,sizeof(buf),1,ffd);
	if(buf[0]=='I' && buf[1]=='D' && buf[2]=='3'){
		return 1;
	}
	fseek(ffd,-(128*sizeof(buf[0])),SEEK_END);
	fread(buf,sizeof(buf),1,ffd);
	if(buf[0]=='T' && buf[1]=='A' && buf[2]=='G'){
		return 1;
	}
	return 0;
}
void new_format(struct playerHandles *ph){
}

int mp3Init(struct playerHandles *ph){
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
		fprintf(stderr,"\nno dechandle\n");
		return;
	}

	struct mp3Handles *h=(struct mp3Handles *)ph->dechandle;
	mpg123_seek_frame(h->m,mpg123_timeframe(h->m,(int)modtime),modtime?SEEK_CUR:SEEK_SET);


	if(modtime){
		*h->total+=(modtime*h->accuracy);
		if(*h->total<0)*h->total=0;
	}
	else
		*h->total=0;

	snd_clear(ph);
}

void plugin_exit(struct playerHandles *ph){
	if(h.m!=NULL)mpg123_delete(h.m);
	mpg123_exit();
}

int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	size_t size;
	ssize_t len;
	int mret=MPG123_NEED_MORE,framesize=4;
	int retval=DEC_RET_SUCCESS;
	
	if(mp3Init(ph)<0)return DEC_RET_ERROR;

	long total=0;
	int samptime=0,accuracy=1000;
	struct outputdetail details;
	details.totaltime=*totaltime>0?*totaltime:-1;
	details.percent=-1;
	//len=fread(buf,sizeof(unsigned char),insize,ph->ffd); // trash first frame
			long ratel;
			int channels, enc,enc_bit=2;
			unsigned int rate;
			mpg123_getformat(h.m, &ratel, &channels, &enc);
			//ratel=44100;channels=2;enc=MPG123_ENC_16;
			rate=(unsigned int)ratel;
			//enc=getMp3EncBytes(&enc_bit);
			fprintf(stderr,"New format: %dHz %d channels %d encoding\n",(int)ratel, channels, enc_bit*8);
			snd_param_init(ph,&enc,&channels,&rate);
			//mpg123_info(h.m,&fi);
			//TODO:don't hardcode the padding byte
			//framesize=ceil((double)fi.framesize/144)+1;
			framesize=channels*enc_bit;//channels*sample_bits
			samptime=rate*framesize;
			//fprintf(stderr,"New Format: framesize samptime %d %d\n",framesize,samptime);

	ph->dechandle=&h;
	h.total=&total;
	h.accuracy=accuracy;
	int outsize=mpg123_outblock(h.m);
	unsigned char *out=alloca(sizeof(unsigned char)*outsize);
	//fprintf(stderr,"Buffers (out): %d\n",outsize);

	do{ /* Read and write until everything is through. */
		//if outbuff isn't big enough to hold all decoded data from inbuff, keep going
		if(mret != MPG123_NEED_MORE){
			//mret = mpg123_decode(h.m,NULL,0,out,outsize,&size);
			mret=mpg123_read(h.m,out,outsize,&len);
		}
		else{
		//	if((len=fread(buf,sizeof(unsigned char),insize,ph->ffd)) <= 0){
			mret=mpg123_read(h.m,out,outsize,&len);
			if(mret==MPG123_DONE || mret==MPG123_ERR){
				// EOF (or read error)
				retval=DEC_RET_SUCCESS;
				fprintf(stderr,"\ndone..\n");
				break;
			}
			/* Feed input chunk and get first chunk of decoded audio. */
			//mret = mpg123_decode(h.m,buf,len,out,outsize,&size);
		}
		if(mret==MPG123_NEW_FORMAT){
			mpg123_getformat(h.m, &ratel, &channels, &enc);
			//ratel=44100;channels=2;enc=MPG123_ENC_16;
			rate=(unsigned int)ratel;
			//enc=getMp3EncBytes(&enc_bit);
			fprintf(stderr,"New format (Hz;channels;encoding): %d %d %d\n",(int)ratel, channels, enc);
			snd_param_init(ph,&enc,&channels,&rate);
			//mpg123_info(h.m,&fi);
			//TODO:don't hardcode the padding byte
			//framesize=ceil((double)fi.framesize/144)+1;
			framesize=channels*enc_bit;//channels*sample_bits
			samptime=rate*framesize;
			fprintf(stderr,"New Format: framesize samptime %d %d\n",framesize,samptime);
			//snd_pcm_prepare(ph->sndfd);
		}
		//if(size==0 || len==0)continue;
		if(len==0)continue;
		size=len;

		details.curtime=total/accuracy;
		details.percent=(details.curtime*100)/details.totaltime;
		crOutput(ph->pflag,&details);
		total+=(size*accuracy)/samptime;

#ifdef HAVE_LIBASOUND
		if(writei_snd(ph,(char *)out,size/framesize)<0)break;
#else
		if(writei_snd(ph,(char *)out,size)<0)break;
#endif

		if((retval=doLocalKey(key))!=DEC_RET_SUCCESS){
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
	*totaltime=details.curtime;
	return retval;
}

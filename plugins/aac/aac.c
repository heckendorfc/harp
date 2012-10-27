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
 *  GNU General Public License for more details->
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "plugin.h"
#include <mp4ff.h>
#include <neaacdec.h>

#define AAC_MAX_CHANNELS (6)

uint32_t read_callback(void *userdata, void *buffer, uint32_t length);
uint32_t seek_callback(void *userdata, uint64_t position);
int GetAACTrack(mp4ff_t *infile);

#include "aacmeta.c"

struct aacHandles{
	unsigned int *total;
	unsigned int *sample;
	unsigned int *rate;
	int framesize;
	int channels;
}h;

FILE * plugin_open(const char *path, const char *mode){
	return plugin_std_fopen(path,mode);
}

void plugin_close(FILE *ffd){
	plugin_std_fclose(ffd);
}

int filetype_by_data(FILE *ffd){
	unsigned char buf[10];
	fseek(ffd,4*sizeof(buf[0]),SEEK_SET);
	if(!fread(buf,sizeof(buf),1,ffd))return 0;
	if(buf[0]=='f' && buf[1]=='t' && buf[2]=='y' && buf[3]=='p'){
		return 1;
	}
	return 0;
}

void plugin_seek(struct playerHandles *ph, int modtime){
	if(ph->dechandle==NULL)return;

	struct aacHandles *h=(struct aacHandles *)ph->dechandle;
	if(modtime==0){
		*h->sample=0;
		*h->total=0;
		snd_clear(ph);
		return;
	}

	*h->total+=modtime*(*h->rate);
	*h->sample+=((*h->rate)*modtime)/(h->framesize);

	if(*h->sample<0 || *h->total<0){
		*h->sample=0;
		*h->total=0;
	}

	snd_clear(ph);
}

uint32_t read_callback(void *userdata, void *buffer, uint32_t length){
	//fprintf(stderr,"Reading %d\n",length);
	return fread(buffer,sizeof(unsigned char),length,(FILE*)userdata);
}

uint32_t seek_callback(void *userdata, uint64_t position){
	//fprintf(stderr,"Seeking %d\n",position);
	return fseek((FILE*)userdata,position,SEEK_SET);
}

int GetAACTrack(mp4ff_t *infile){
	int i,ret;
	int numtracks=mp4ff_total_tracks(infile);

	for(i=0;i<numtracks;i++){
		unsigned char *buff=NULL;
		unsigned int buffsize=0;

		mp4AudioSpecificConfig mp4cfg;
		mp4ff_get_decoder_config(infile,i,&buff,&buffsize);

		if(buff){
			ret=NeAACDecAudioSpecificConfig(buff,buffsize,&mp4cfg);
			free(buff);

			if(ret<0)
				continue;
			return i;
		}
	}
	return -1;
}

/**
 * Check whether the buffer head is an AAC frame, and return the frame
 * length.  Returns 0 if it is not a frame.
 */
static size_t adts_check_frame(const unsigned char *data){
	/* check syncword */
	if (!((data[0] == 0xFF) && ((data[1] & 0xF6) == 0xF0)))
		return 0;

	return (((unsigned int)data[3] & 0x3) << 11) |
		(((unsigned int)data[4]) << 3) |
		(data[5] >> 5);
}

static size_t fill_buffer(FILE *ffd, char *buf, const int buf_len){
	size_t length=fread(buf,1,buf_len,ffd);
	//if(length>0)fprintf(stderr,"\nRead %d\n",length);
	return length;
}

/**
 * Find the next AAC frame in the buffer.  Returns 0 if no frame is
 * found or if not enough data is available.
 */
static size_t adts_find_frame(FILE *ffd, char *buf, const int start, const int buf_len){
	char *data, *p, *bp;
	size_t bp_len, length, frame_length, next_frame;

	bp=buf;
	bp_len=0;
	length=0;

	bp_len=start+fill_buffer(ffd,bp+start,buf_len-start);
	while (1) {
		if(bp_len<8){
			if(bp_len>0)
				memmove(buf,bp,bp_len);
			usleep(100000);
			bp_len+=fill_buffer(ffd,buf+bp_len,buf_len-bp_len);
			bp=buf;
			continue;
		}

		p = memchr(bp, 0xff, bp_len);
		if (p == NULL){
			/* No marker found. clear buffer */
			bp=buf;
			bp_len=0;
			continue;
		}

		if(p>bp){
			/* discard data before marker */
			bp_len-=(p-bp);
			bp=p;
		}

		/* is it a frame? */
		frame_length = adts_check_frame(bp);
		next_frame = adts_check_frame(bp+frame_length);
		if (frame_length == 0 || next_frame == 0) {
			/* it's just some random 0xff byte; discard it
			   and continue searching */
			bp++;
			bp_len--;
			continue;
		}

		if(bp_len<frame_length){
			/* we don't have a full frame in the buffer. */
			if(buf_len<frame_length){
				/* ... and we never will. Clear the buffer and look for the next frame. */
					/* Should we give up at this point? */
				fprintf(stderr,"AAC | Buffer too small for this frame\n");
				bp=buf;
				bp_len=0;
				continue;
			}
			
			memmove(buf,bp,bp_len);
			while(bp_len<frame_length){
				int i;
				usleep(100000);
				bp_len+=i=fill_buffer(ffd,buf+bp_len,buf_len-bp_len);
			}
		}
		else
			memmove(buf,bp,bp_len);

		return frame_length;
	}
	return 0;
}

int decodeAAC(struct playerHandles *ph, char *key, int *totaltime, char *buf, const int buf_filled, const int bufsize){
	char *out;
	int track,fmt,ret,channels,retval=DEC_RET_SUCCESS;
	unsigned int rate;
	unsigned char channelchar;
	unsigned long ratel;
	char tail[OUTPUT_TAIL_SIZE];
	NeAACDecFrameInfo hInfo;
	unsigned int total=0;
	struct outputdetail *details=ph->outdetail;
	size_t frame_size;
	int bufstart=0;
	uint32_t adts_header=0;

	NeAACDecHandle hAac = NeAACDecOpen();
	NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(hAac);

	config->useOldADTSFormat=0;
	//config->defObjectType=LC;
	config->outputFormat = FAAD_FMT_16BIT;
#ifdef HAVE_FAACDECCONFIGURATION_DOWNMATRIX
	config->downMatrix = 1;
#endif
#ifdef HAVE_FAACDECCONFIGURATION_DONTUPSAMPLEIMPLICITSBR
	config->dontUpSampleImplicitSBR = 0;
#endif

	if(NeAACDecSetConfiguration(hAac,config)==0){
		fprintf(stderr,"set conf failed");
		return DEC_RET_ERROR;
	}

	frame_size=adts_find_frame(ph->ffd,buf,buf_filled,bufsize);
	adts_header=(((uint32_t*)buf)[0]) >> 6;
	
	if((ret=NeAACDecInit(hAac,buf,bufsize,&ratel,&channelchar)) == 0){
		channels=(int)channelchar;
		fmt=(int)config->outputFormat;
		rate=(unsigned int)ratel;
	}
	else{
		fprintf(stderr,"NeAACDecInit error %d\n",ret);
		channels=2;
		rate=44100;
	}
	//fprintf(stderr,"Init returned %d\n",ret);

	snprintf(tail,OUTPUT_TAIL_SIZE,"New format: %dHz %dch",rate, channels);
	addStatusTail(tail,ph->outdetail);

	snd_param_init(ph,&fmt,&channels,&rate);

	details->totaltime=*totaltime;

	h.total=&total;
	//h.sample=&sample;
	h.rate=&rate;
	h.framesize=frame_size;
	h.channels=channels;

	ph->dechandle=&h;


#if WITH_ALSA==1
	#define OUTSIZE(x) (x)
#else
	#define OUTSIZE_AAC(x) (x*channels)
#endif

	do{
		if(0 && adts_header != (((uint32_t*)buf)[0]) >> 6){
			adts_header=(((uint32_t*)buf)[0]) >> 6;
			NeAACDecClose(hAac);
			if((ret=NeAACDecInit(hAac,buf,bufsize,&ratel,&channelchar)) == 0){
				channels=(int)channelchar;
				fmt=(int)config->outputFormat;
				rate=(unsigned int)ratel;
			}
			else{
				fprintf(stderr,"New format init failed.\n");
			}
		}
		out=(char *)NeAACDecDecode(hAac,&hInfo,buf,bufsize);

		if(hInfo.error>0){
			fprintf(stderr,"AAC | Error while decoding - %d: %s\n",hInfo.error,NeAACDecGetErrorMessage(hInfo.error));
			//retval=DEC_RET_ERROR;
			//break;
		}
		else if(hInfo.samples>0){
			total+=hInfo.samples/channels; // framesize?
			if(writei_snd(ph,out,OUTSIZE_AAC(hInfo.samples))<0)break;

			details->curtime=total/rate;
			//details->percent=(sample*100)/numsamples;
		}

		if(ph->pflag->exit!=DEC_RET_SUCCESS){
			retval=ph->pflag->exit;
			break;	
		}

		memmove(buf,buf+frame_size,bufsize-frame_size);
		bufstart=bufsize-frame_size;
		frame_size=adts_find_frame(ph->ffd,buf,bufstart,bufsize);
		if(frame_size==0){
			fprintf(stderr,"\nframe_size==0\n");
			retval=DEC_RET_ERROR;
			break;
		}
	}while(1);

	free(buf);
	NeAACDecClose(hAac);
	*totaltime=details->curtime;
	return retval;

}

int decodeMP4(struct playerHandles *ph, char *key, int *totaltime, char *o_buf, const int buf_filled, const int buf_len){
	unsigned char *buf=NULL;
	char *out;
	ssize_t len;
	int track,fmt,ret,channels,retval=DEC_RET_SUCCESS;
	unsigned int rate,bufsize;
	mp4ff_t infile;
	
	mp4ff_callback_t *mp4cb;
	if(!(mp4cb=malloc(sizeof(mp4ff_callback_t)))){
		fprintf(stderr,"Malloc failed (mp4cb).");
		return DEC_RET_ERROR;
	}
	mp4cb->read=read_callback;
	mp4cb->seek=seek_callback;
	mp4cb->user_data=ph->ffd;

	infile=mp4ff_open_read(mp4cb);
	if(!infile){
		fprintf(stderr,"mp4ffopenread failed");
		free(mp4cb);
		return DEC_RET_ERROR;
	}

	if((track=GetAACTrack(infile))<0){
		fprintf(stderr,"getaactrack failed");
		mp4ff_close(infile);
		free(mp4cb);
		return DEC_RET_ERROR;
	}

	NeAACDecFrameInfo hInfo;
	//unsigned long ca = NeAACDecGetCapabilities();
	//fprintf(stderr,"%x %x %x %x %x %x\n",ca&LC_DEC_CAP,ca&MAIN_DEC_CAP,ca&LTP_DEC_CAP,ca&LD_DEC_CAP,ca&ERROR_RESILIENCE_CAP,ca&FIXED_POINT_CAP);
	NeAACDecHandle hAac = NeAACDecOpen();
	NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(hAac);
		/* TODO: Change values if needed. 
	conf->defObjectType=LC;
	conf->outputFormat=FAAD_FMT_16BIT;
	conf->defSampleRate=44100;
	conf->downMatrix=1;*/
	if(NeAACDecSetConfiguration(hAac,conf)==0){
		fprintf(stderr,"set conf failed");
		return DEC_RET_ERROR;
	}

	mp4ff_get_decoder_config(infile,track,&buf,&bufsize);

	unsigned char channelchar;
	unsigned long ratel;
	char tail[OUTPUT_TAIL_SIZE];
	if((ret=NeAACDecInit2(hAac,buf,bufsize,&ratel,&channelchar)) == 0){
		channels=(int)channelchar;
		fmt=(int)conf->outputFormat;
		rate=(unsigned int)ratel;
	}
	else{
		fprintf(stderr,"NeAACDecInit2 error %d\n",ret);
		channels=2;
		rate=44100;
	}
	snprintf(tail,OUTPUT_TAIL_SIZE,"New format: %dHz %dch",rate, channels);
	addStatusTail(tail,ph->outdetail);

	mp4AudioSpecificConfig mp4cfg;
	unsigned int framesize=1024;
	if(buf){
		if(NeAACDecAudioSpecificConfig(buf,len,&mp4cfg)>=0){
			if(mp4cfg.frameLengthFlag==1)framesize=960;
			if(mp4cfg.sbr_present_flag==1)framesize<<=1;
		}
		free(buf);
	}
	//fprintf(stderr,"framesize: %d\n",framesize);

	snd_param_init(ph,&fmt,&channels,&rate);

	unsigned int total=0,sample,numsamples=mp4ff_num_samples(infile,track);
	struct outputdetail *details=ph->outdetail;
	details->totaltime=*totaltime;

	h.total=&total;
	h.sample=&sample;
	h.rate=&rate;
	h.framesize=framesize;
	h.channels=channels;

	ph->dechandle=&h;

#if WITH_ALSA==1
	const int outsize=framesize;
#else
	const int outsize=framesize*channels*2;
#endif

	//fprintf(stderr,"numsamples: %d\n",numsamples);
	for(sample=0;sample<numsamples;sample++){ 
		ret=mp4ff_read_sample(infile,track,sample,&buf,&bufsize);
		if(ret==0){
			fprintf(stderr,"error reading sample\n");
			retval=DEC_RET_ERROR;
			break;
		}
		out=(char *)NeAACDecDecode(hAac,&hInfo,buf,bufsize);

		total+=hInfo.samples/channels; // framesize?
		if(hInfo.error>0){
			fprintf(stderr,"Error while decoding %d %s\n",hInfo.error,NeAACDecGetErrorMessage(hInfo.error));
			retval=DEC_RET_ERROR;
			break;
		}
		if(hInfo.samples<1)continue;

		if(writei_snd(ph,out,outsize)<0)break;

		details->curtime=total/rate;
		details->percent=(sample*100)/numsamples;

		if(ph->pflag->exit!=DEC_RET_SUCCESS){
			retval=ph->pflag->exit;
			break;	
		}
	}

	/* Done decoding, now just clean up and leave. */
	mp4ff_close(infile);
	free(mp4cb);
	NeAACDecClose(hAac);
	*totaltime=details->curtime;
	return retval;
}

int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	char *buf=NULL;
	int bufsize=FAAD_MIN_STREAMSIZE * AAC_MAX_CHANNELS;
	int frame_size;

	if(!(buf=malloc(bufsize)))
		return DEC_RET_ERROR;
	
	frame_size=fill_buffer(ph->ffd,buf,8);

	if(buf[4]=='f' && buf[5]=='t' && buf[6]=='y' && buf[7]=='p'){
		fseek(ph->ffd,0,SEEK_SET);
		return decodeMP4(ph,key,totaltime,buf,frame_size,bufsize);
	}
	else{
		return decodeAAC(ph,key,totaltime,buf,frame_size,bufsize);
	}
}

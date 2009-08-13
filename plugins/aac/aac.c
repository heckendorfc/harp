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
#include <mp4ff.h>
#include <neaacdec.h>
#include "aacmeta.c"

struct aacHandles{
	int *total;
	int *sample;
	int *rate;
	int framesize;
	int channels;
}h;

int filetype_by_data(FILE *ffd){
	unsigned char buf[10];
	fseek(ffd,4*sizeof(buf[0]),SEEK_SET);
	fread(buf,sizeof(buf),1,ffd);
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
	return fread(buffer,sizeof(unsigned char),length,(FILE*)userdata);
}

uint32_t seek_callback(void *userdata, uint64_t position){
	return fseek((FILE*)userdata,position,SEEK_SET);
}

static int GetAACTrack(mp4ff_t *infile){
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

int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	unsigned char *buf=NULL,*out; /* buffers  */
	ssize_t len;
	int track,fmt,ret,channels,retval=DEC_RET_SUCCESS;
	unsigned int rate,bufsize;
	mp4ff_t infile;
	
	mp4ff_callback_t *mp4cb = malloc(sizeof(mp4ff_callback_t));
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
	if((ret=NeAACDecInit2(hAac,buf,bufsize,&ratel,&channelchar)) == 0){
		//fprintf(stderr,"New format (Hz;channels;encoding): %d %d %d\n",(int)ratel, (int)channelchar, (int)conf->outputFormat);
		fprintf(stderr,"New format: %dHz %d channels\n",(int)ratel, (int)channelchar);
		channels=(int)channelchar;
		fmt=(int)conf->outputFormat;
		rate=(unsigned int)ratel;
		//memcpy(buf,&buf[ret-1],len-ret);
	}
	else{
		fprintf(stderr,"NeAACDecInit2 error %d\n",ret);
		channels=2;
		rate=44100;
		fprintf(stderr,"New format: %dHz %d channels\n",(int)ratel, (int)channelchar);
	}

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
	struct outputdetail details;
	details.totaltime=*totaltime;

	h.total=&total;
	h.sample=&sample;
	h.rate=&rate;
	h.framesize=framesize;
	h.channels=channels;

	ph->dechandle=&h;

	//fprintf(stderr,"numsamples: %d\n",numsamples);
	for(sample=0;sample<numsamples;sample++){ 
		ret=mp4ff_read_sample(infile,track,sample,&buf,&bufsize);
		if(ret==0){
			fprintf(stderr,"error reading sample\n");
			retval=DEC_RET_ERROR;
			break;
		}
		out=(unsigned char *)NeAACDecDecode(hAac,&hInfo,buf,bufsize);

		total+=hInfo.samples/channels; // framesize?
		if(hInfo.error>0){
			fprintf(stderr,"Error while decoding %d %s\n",hInfo.error,NeAACDecGetErrorMessage(hInfo.error));
			retval=DEC_RET_ERROR;
			break;
		}
		if(hInfo.samples<1)continue;
#ifdef HAVE_LIBASOUND
		if(writei_snd(ph,(char *)out,framesize)<0)break;
#else
		if(writei_snd(ph,(char *)out,framesize*channels*2)<0)break;
#endif

		details.curtime=total/rate;
		details.percent=(sample*100)/numsamples;
		crOutput(ph->pflag,&details);

		if((retval=doLocalKey(key))!=DEC_RET_SUCCESS){
			break;
		}
	}

	/* Done decoding, now just clean up and leave. */
	mp4ff_close(infile);
	free(mp4cb);
	NeAACDecClose(hAac);
	*totaltime=details.curtime;
	return retval;
}

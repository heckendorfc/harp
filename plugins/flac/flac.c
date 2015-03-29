/*
 *  Copyright (C) 2009-2015  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "../plugin.h"
#include "flacmeta.c"
#include <FLAC/all.h>

struct snd_data{
	struct playerHandles *ph;
	FLAC__StreamDecoder *decoder;
	int size;
	int channels;
	int enc;
	unsigned int rate;
	unsigned int curtime;
	int *totaltime;
	volatile unsigned int nextpos;
	volatile char seeking;
};

static FILE * plugin_open(const char *path, const char *mode){
	return plugin_std_fopen(path,mode);
}

static void plugin_close(FILE *ffd){
	//plugin_std_fclose(ffd);
}

static void plugin_seek(struct playerHandles *ph, int modtime){
	int modshift;
	struct snd_data *data;

	if(ph->dechandle==NULL)
		return;

	data=(struct snd_data*)ph->dechandle;
	modshift=-(modtime*data->rate);
	if(modtime==0 || (modshift>0 && data->curtime<modshift))
		data->nextpos=0;
	else
		data->nextpos=data->curtime-modshift;
	data->seeking=1;
/*
	struct vorbisHandles *h=(struct vorbisHandles *)ph->dechandle;

	int seconds=(*h->total)/((h->rate)*(h->sizemod));
	seconds+=modtime;

	if(ov_time_seek(h->vf,seconds)!=0)
		return;
	*h->total=seconds*((h->rate)*(h->sizemod));

	if(*h->total<0)
		*h->total=0;

	snd_clear(ph);
*/
}

#if WITH_ALSA==1
#define FLAC_SIZE(x) (x)
#else
#define FLAC_SIZE(x) (x*by*data->channels)
#endif

static FLAC__StreamDecoderWriteStatus flac_write(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data){
	struct snd_data *data=(struct snd_data*)client_data;
	//if(writen_snd(data->ph, (void**)bufs, frame->header.blocksize)<0)return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	//int x;for(x=0;x<frame->header.blocksize;x++)if(writei_snd(data->ph, (char *)&buffer[0][x], 1)<0 || writei_snd(data->ph, (char *)&buffer[1][x],1)<0)return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	char *buffs;
	const int by=data->size/8;
	int *bt;
	int i,j,num;
	//const int by=2;
	if(!(buffs=malloc(frame->header.blocksize*by*data->channels*2))){ // *2 ~just in case~ ? try to fix sbrk segfault
		fprintf(stderr,"Malloc failed (decoder buffer)");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	num=frame->header.blocksize;
	for(i=0;i<num;i++){
		for(j=0;j<data->channels;j++){
			bt=(int*)(buffs+i*by*data->channels+j*by);
			*bt=buffer[j][i];
		}
		//buffs[i*4+2]=buffer[1][i];
		//buffs[i*4+3]=buffer[1][i]>>8;
	}
	writei_snd(data->ph, buffs, FLAC_SIZE(frame->header.blocksize)); // TODO: This works for ALSA. Other sources need something else for size?
	//data->curtime+=frame->header.blocksize;
	data->curtime=frame->header.number.sample_number;
	//fprintf(stderr,"%d ",frame->header.blocksize);
	//writei_snd(data->ph, (void *)buffer[0], frame->header.blocksize);
	free(buffs);
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void flac_meta(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data){
	struct snd_data *data=(struct snd_data*)client_data;
	data->channels=metadata->data.stream_info.channels;
	data->rate=metadata->data.stream_info.sample_rate;
	data->size=metadata->data.stream_info.bits_per_sample;
	//fprintf(stderr,"samps %li\n",(long int)metadata->data.stream_info.total_samples);
	*data->totaltime=metadata->data.stream_info.total_samples/data->rate;
}

static void flac_error(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data){

}

static int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	struct snd_data data;
	unsigned int retval=DEC_RET_SUCCESS;
	struct outputdetail *details=ph->outdetail;
	char tail[OUTPUT_TAIL_SIZE];

	data.ph=ph;
	data.totaltime=totaltime;
	data.curtime=0;
	data.nextpos=0;
	data.seeking=0;

	FLAC__StreamDecoder *decoder=NULL;
	ph->dechandle=&data;
	//FLAC__StreamDecoderInitStatus status;
	//int status=0;

	if((decoder=FLAC__stream_decoder_new())==NULL){
		fprintf(stderr,"flac decoder alloc failed");
		return DEC_RET_ERROR;
	}

	FLAC__stream_decoder_set_metadata_ignore_all(decoder);
	FLAC__stream_decoder_set_metadata_respond(decoder,FLAC__METADATA_TYPE_STREAMINFO);
	//FLAC__stream_decoder_set_metadata_respond(decoder,FLAC__METADATA_TYPE_SEEKTABLE); //when seeking is implemented

	if(FLAC__stream_decoder_init_FILE(decoder,ph->ffd,flac_write,flac_meta,flac_error,&data)!=FLAC__STREAM_DECODER_INIT_STATUS_OK){
		fprintf(stderr,"flac init failed");
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
		return DEC_RET_ERROR;
	}

	if(!FLAC__stream_decoder_process_until_end_of_metadata(decoder)){
		fprintf(stderr,"flac decoder metadata failed");
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
		return DEC_RET_ERROR;
	}
	//fprintf(stderr,"New format: %d Hz, %i channels, encoding value %d\n", data.rate, data.channels, data.size);
	snprintf(tail,OUTPUT_TAIL_SIZE,"New format: %dHz %dch %dbit",data.rate, data.channels, data.size);
	addStatusTail(tail,ph->outdetail);
	snd_param_init(ph,&data.size,&data.channels,&data.rate);

	if((details->totaltime=*data.totaltime)==0)
		details->totaltime=-1;;
	details->percent=-1;

	do{ /* Read and write until everything is through. */
		if(data.seeking){
			if(!FLAC__stream_decoder_seek_absolute(decoder,data.nextpos)){
				if(data.nextpos>=FLAC__stream_decoder_get_total_samples(decoder))
					break;
				else
					fprintf(stderr,"\nSeek to %d failed\n",data.nextpos);
			}
			data.seeking=0;
		}
		if(FLAC__stream_decoder_process_single(decoder)==(FLAC__bool)false){fprintf(stderr,"Early abort\n");break;}
		//if((size=ov_read(vf,buf,len,0,2,1,&vf->current_link))<1){fprintf(stderruh oh");break;}

		//data.curtime+=data.size;

		details->curtime=data.curtime/data.rate; // Not perfect, but close enough
		details->percent=(details->curtime*100)/details->totaltime;
		crOutput(ph->pflag,details);

		if(ph->pflag->exit!=DEC_RET_SUCCESS){
			retval=ph->pflag->exit;
			break;
		}
	}while(FLAC__stream_decoder_get_state(decoder)!=FLAC__STREAM_DECODER_END_OF_STREAM);
	//fprintf(stderr,"total: %d\n",data.curtime);
	writei_snd(ph,NULL,0); // drain sound buffer

	/* Done decoding, now just clean up and leave. */
	FLAC__stream_decoder_finish(decoder);
	FLAC__stream_decoder_delete(decoder);

	return retval;
}

static int filetype_by_data(FILE *ffd){
	unsigned char buf[10];
	fseek(ffd,4*sizeof(buf[0]),SEEK_SET);
	if(!fread(buf,sizeof(buf),1,ffd))return 0;
	if(buf[0]=='f' && buf[1]=='L' && buf[2]=='a' && buf[3]=='C'){
		return 1;
	}
	return 0;
}

struct pluginitem flacitem={
	.modopen=plugin_open,
	.modclose=plugin_close,
	.moddata=filetype_by_data,
	.modplay=plugin_run,
	.modseek=plugin_seek,
	.modmeta=flac_plugin_meta,
	.contenttype="x-flac",
	.extension={"flac",NULL},
	.name="FLAC",
};

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

#include "plugin.h"
#include <FLAC/all.h>

struct snd_data{
	struct playerHandles *ph;
	int size;
	int channels;
	int enc;
	unsigned int rate;
	unsigned int curtime;
	int *totaltime;
};

void plugin_seek(struct playerHandles *ph, int modtime){
	if(ph->dechandle==NULL)return;
	return;
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

FLAC__StreamDecoderWriteStatus flac_write(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data){
	struct snd_data *data=(struct snd_data*)client_data;
	//if(writen_snd(data->ph, (void**)bufs, frame->header.blocksize)<0)return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	//int x;for(x=0;x<frame->header.blocksize;x++)if(writei_snd(data->ph, (char *)&buffer[0][x], 1)<0 || writei_snd(data->ph, (char *)&buffer[1][x],1)<0)return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	char *buffs;
	if(!(buffs=malloc(frame->header.blocksize*4))){
		debug(2,"Malloc failed (decoder buffer)");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	int x;
	for(x=0;x<frame->header.blocksize;x++){
		buffs[x*4]=buffer[0][x];
		buffs[x*4+1]=buffer[0][x]>>8;
		buffs[x*4+2]=buffer[1][x];
		buffs[x*4+3]=buffer[1][x]>>8;
	}
	writei_snd(data->ph, buffs, frame->header.blocksize);
	data->curtime+=frame->header.blocksize;
	//fprintf(stderr,"%d ",frame->header.blocksize);
	//writei_snd(data->ph, (void *)buffer[0], frame->header.blocksize);
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void flac_meta(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data){
	struct snd_data *data=(struct snd_data*)client_data;
	data->channels=metadata->data.stream_info.channels;
	data->rate=metadata->data.stream_info.sample_rate;
	data->size=metadata->data.stream_info.bits_per_sample;
	//fprintf(stderr,"samps %li\n",(long int)metadata->data.stream_info.total_samples);
	*data->totaltime=metadata->data.stream_info.total_samples/data->rate;
}

void flac_error(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data){

}

int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	struct snd_data data;
	data.ph=ph;
	data.totaltime=totaltime;
	data.curtime=0;

	FLAC__StreamDecoder *decoder=NULL;
	//FLAC__StreamDecoderInitStatus status;
	//int status=0;

	if((decoder=FLAC__stream_decoder_new())==NULL){
		debug(2,"flac decoder alloc failed");
		return DEC_RET_ERROR;
	}

	FLAC__stream_decoder_set_metadata_ignore_all(decoder);
	FLAC__stream_decoder_set_metadata_respond(decoder,FLAC__METADATA_TYPE_STREAMINFO);
	//FLAC__stream_decoder_set_metadata_respond(decoder,FLAC__METADATA_TYPE_SEEKTABLE); //when seeking is implemented
	
	if(FLAC__stream_decoder_init_FILE(decoder,ph->ffd,flac_write,flac_meta,flac_error,&data)!=FLAC__STREAM_DECODER_INIT_STATUS_OK){
		debug(2,"flac init failed");
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
		return DEC_RET_ERROR;
	}

	unsigned int retval=DEC_RET_SUCCESS,total=0;
	struct outputdetail details;
	details.totaltime=*totaltime;
	details.percent=-1;


	if(!FLAC__stream_decoder_process_until_end_of_metadata(decoder)){
		debug(2,"flac decoder metadata failed");
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
		return DEC_RET_ERROR;
	}
	//fprintf(stderr,"New format: %d Hz, %i channels, encoding value %d\n", data.rate, data.channels, data.size);
	fprintf(stderr,"New format: %dHz %d channels %d encoding\n",data.rate, data.channels, data.size);

	snd_param_init(ph,&data.enc,&data.channels,&data.rate);

	do{ /* Read and write until everything is through. */
		if(FLAC__stream_decoder_process_single(decoder)==(FLAC__bool)false){debug(2,"uh oh");break;}
		//if((size=ov_read(vf,buf,len,0,2,1,&vf->current_link))<1){debug("uh oh");break;}

		//data.curtime+=data.size;

		details.curtime=data.curtime/data.rate; // Not perfect, but close enough
		details.percent=(details.curtime*100)/details.totaltime;
		crOutput(ph->pflag,&details);

		if((retval=doLocalKey(key))!=DEC_RET_SUCCESS){
			break;	
		}
	}while(FLAC__stream_decoder_get_state(decoder)!=FLAC__STREAM_DECODER_END_OF_STREAM);
	//fprintf(stderr,"total: %d\n",data.curtime);

	/* Done decoding, now just clean up and leave. */
	FLAC__stream_decoder_finish(decoder);
	FLAC__stream_decoder_delete(decoder);
	
	return retval;
}

void plugin_meta(FILE *ffd, struct musicInfo *mi){
	return;
}

int filetype_by_data(FILE *ffd){
	unsigned char buf[10];
	fseek(ffd,4*sizeof(buf[0]),SEEK_SET);
	fread(buf,sizeof(buf),1,ffd);
	if(buf[0]=='f' && buf[1]=='L' && buf[2]=='a' && buf[3]=='C'){
		return 1;
	}
	return 0;
}

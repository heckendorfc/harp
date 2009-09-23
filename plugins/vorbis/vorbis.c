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
#include <vorbis/vorbisfile.h>
#include "vorbismeta.c"

struct vorbisHandles{
	OggVorbis_File *vf;
	int *total;
	int rate;
	int sizemod;
}h;

int filetype_by_data(FILE *ffd){
	unsigned char buf[10];
	fseek(ffd,0,SEEK_SET);
	fread(buf,sizeof(buf),1,ffd);
	if(buf[0]=='O' && buf[1]=='g' && buf[2]=='g' && buf[3]=='S'){
		return 1;
	}
	return 0;
}

void plugin_seek(struct playerHandles *ph, int modtime){
	if(ph->dechandle==NULL)return;

	struct vorbisHandles *h=(struct vorbisHandles *)ph->dechandle;
	if(modtime==0){
		ov_time_seek(h->vf,0);
		*h->total=0;
		snd_clear(ph);
		return;
	}

	int seconds=(*h->total)/((h->rate)*(h->sizemod));
	seconds+=modtime;

	if(ov_time_seek(h->vf,seconds)!=0)
		return;
	*h->total=seconds*((h->rate)*(h->sizemod));

	if(*h->total<0)
		*h->total=0;

	snd_clear(ph);
}

int vorbStatus(int ret){
	fprintf(stderr,"\n");
	switch(ret){
		case 0:fprintf(stderr,"EOF - done\n");return DEC_RET_SUCCESS;
		case OV_HOLE:fprintf(stderr,"OV_HOLE - data interruption\n");break;
		case OV_EBADLINK:fprintf(stderr,"OV_EBADLINK - invalid stream\n");break;
		case OV_EINVAL:fprintf(stderr,"OV_EINVAL - read or open error\n");break;
		default:fprintf(stderr,"unknown\n");
	}
	return DEC_RET_ERROR;
}

void silencer(){
	OggVorbis_File vf;
	if(ov_open_callbacks(0,&vf,NULL,0,OV_CALLBACKS_DEFAULT)<0);
	if(ov_open_callbacks(0,&vf,NULL,0,OV_CALLBACKS_STREAMONLY)<0);
	if(ov_open_callbacks(0,&vf,NULL,0,OV_CALLBACKS_STREAMONLY_NOCLOSE)<0);
}

int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	size_t size;
	char buf[1600];  /* input buffer  */
	ssize_t len=1600;
	//int ret,framesize=4;
	
	//snd_pcm_uframes_t frames;
	OggVorbis_File *vf=malloc(sizeof(OggVorbis_File));
	//int status=0;

	if(ov_open_callbacks(ph->ffd,vf,NULL,0,OV_CALLBACKS_NOCLOSE)<0){
		fprintf(stderr,"ov open failed\n");
		free(vf);
		return DEC_RET_ERROR;
		//snd_pcm_drop(ph->sndfd);
	}

	unsigned int total=0,sizemod;
	int channels, enc, retval=DEC_RET_SUCCESS;
	unsigned int rate;
	vorbis_info *vi;
	struct outputdetail details;
	details.totaltime=*totaltime;
	details.percent=-1;

	vi=ov_info(vf,-1);
	rate=(unsigned int)vi->rate;
	channels=(unsigned int)vi->channels;
	sizemod=2*channels;
	//fprintf(stderr,"New format (Hz;channels;encoding): %d %d %d\n",rate, channels, (int)vi->bitrate_nominal);
	fprintf(stderr,"New format: %dHz %d channels %d encoding\n",rate, channels, (int)vi->bitrate_nominal);
	snd_param_init(ph,&enc,&channels,&rate);

	h.vf=vf;
	h.total=&total;
	h.rate=rate;
	h.sizemod=sizemod;
	ph->dechandle=&h;

	do{ /* Read and write until everything is through. */
		if((size=ov_read(vf,buf,len,0,2,1,&vf->current_link))<1){retval=vorbStatus(size);break;}
		details.curtime=total/(rate*sizemod);
		details.percent=(details.curtime*100)/details.totaltime;
		crOutput(ph->pflag,&details);

#if WITH_ALSA==1
		if(writei_snd(ph,buf,size/sizemod)<0)break; //(datasize)/(encodeBytes*channels)
#else
		if(writei_snd(ph,buf,size)<0)break; //(datasize)/(encodeBytes*channels)
#endif
		total+=size;

		if((retval=doLocalKey(key))!=DEC_RET_SUCCESS){
			break;
		}
	}while(1);

	/* Done decoding, now just clean up and leave. */
	ov_clear(vf);
	*totaltime=details.curtime;
	return retval;
}

/*
 *  Copyright (C) 2009-2014  Christian Heckendorf <heckendorfc@gmail.com>
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

#include "alsautil.h"
#include "sndutil.h"

int snd_init(struct playerHandles *ph){
	if(!ph->device)ph->device=strdup("default");
	if(snd_pcm_open(&ph->sndfd,ph->device,SND_PCM_STREAM_PLAYBACK,0)<0){
		fprintf(stderr,"sndfd open failed\n");
		return 1;
	}
	return 0;
}

int snd_param_init(struct playerHandles *ph, int *enc, int *channels, unsigned int *rate){
	int x=0;
	*enc=SND_PCM_FORMAT_S16_LE;
	snd_pcm_drop(ph->sndfd);
	snd_pcm_hw_params_malloc(&ph->params);
	if(ph->params==NULL){
		fprintf(stderr,"can't malloc params\n");
		return 1;
	}
	if(snd_pcm_hw_params_any(ph->sndfd,ph->params)<0)fprintf(stderr,"can't init params\n");
	if(snd_pcm_hw_params_set_access(ph->sndfd,ph->params,SND_PCM_ACCESS_RW_INTERLEAVED)<0)fprintf(stderr,"no access\n");
	if(snd_pcm_hw_params_set_format(ph->sndfd,ph->params,*enc)<0)fprintf(stderr,"can't set fmt\n");
	if(snd_pcm_hw_params_set_channels(ph->sndfd,ph->params,*channels)<0)fprintf(stderr,"can't set channels\n");
	if(snd_pcm_hw_params_set_rate_near(ph->sndfd,ph->params,rate,0)<0)fprintf(stderr,"can't set rate\n");
	if((x=snd_pcm_hw_params(ph->sndfd,ph->params))<0)fprintf(stderr,"can't set parms: %s\n",snd_strerror(x));
	snd_pcm_hw_params_free(ph->params);
	return 0;
}

void changeVolume(struct playerHandles *ph, int mod){
	long val;
	int err;
	float range_p;
	long cur_vol,new_volume;
	long vmin,vmax;
	char tail[OUTPUT_TAIL_SIZE];
	snd_mixer_t *handle;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid,0);
	snd_mixer_selem_id_set_name(sid,"PCM");

	if(snd_mixer_open(&handle,0)<0)
		return;

	if(snd_mixer_attach(handle,"default")<0){
		snd_mixer_close(handle);
		return;
	}

	if(snd_mixer_selem_register(handle,NULL,NULL)<0){
		snd_mixer_close(handle);
		return;
	}

	if(snd_mixer_load(handle)<0){
		snd_mixer_close(handle);
		return;
	}

	elem=snd_mixer_find_selem(handle,sid);
	if(!elem){
		snd_mixer_close(handle);
		return;
	}

	snd_mixer_selem_get_playback_volume_range(elem,&vmin,&vmax);
	range_p = (100.0f/(float)(vmax-vmin));

	snd_mixer_selem_get_playback_volume(elem,0,&cur_vol);
	new_volume = cur_vol+vmin+mod/range_p;
	if(new_volume==cur_vol && mod!=0)new_volume+=(mod<0?-1:1);
	if(new_volume<vmin)new_volume=vmin;
	if(new_volume>vmax)new_volume=vmax;
	if(snd_mixer_selem_set_playback_volume(elem,0,new_volume)<0){
		snd_mixer_close(handle);
		return;
	}

	sprintf(tail,"Volume: %d%%",(int)((float)range_p*(float)new_volume));
	addStatusTail(tail,ph->outdetail);

	snd_mixer_selem_get_playback_volume(elem,1,&cur_vol);
	new_volume = cur_vol+vmin+mod/range_p;
	if(new_volume==cur_vol && mod!=0)new_volume+=(mod<0?-1:1);
	if(new_volume<vmin)new_volume=vmin;
	if(new_volume>vmax)new_volume=vmax;
	if(snd_mixer_selem_set_playback_volume(elem,1,new_volume)<0){
		snd_mixer_close(handle);
		return;
	}

	snd_mixer_close(handle);
}

void toggleMute(struct playerHandles *ph, int *mute){
	long val;
	int err;
	float range_p;
	long current=*mute;
	long vmin,vmax;
	char tail[OUTPUT_TAIL_SIZE];
	snd_mixer_t *handle;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid,0);
	snd_mixer_selem_id_set_name(sid,"PCM");

	if(snd_mixer_open(&handle,0)<0)
		return;

	if(snd_mixer_attach(handle,"default")<0){
		snd_mixer_close(handle);
		return;
	}

	if(snd_mixer_selem_register(handle,NULL,NULL)<0){
		snd_mixer_close(handle);
		return;
	}

	if(snd_mixer_load(handle)<0){
		snd_mixer_close(handle);
		return;
	}

	elem=snd_mixer_find_selem(handle,sid);
	if(!elem){
		snd_mixer_close(handle);
		return;
	}

	snd_mixer_selem_get_playback_volume_range(elem,&vmin,&vmax);
	range_p = (100.0f/(float)(vmax-vmin));
	if(*mute>0){ // Unmute and perform volume change
		*mute=0;
		sprintf(tail,"Volume: %ld%%",current);
		addStatusTail(tail,ph->outdetail);
		current=current/range_p+vmin;
	}
	else{ // Mute
		snd_mixer_selem_get_playback_volume(elem,0,&current);
		*mute=current*range_p+vmin;
		current=0;
		addStatusTail("Volume Muted",ph->outdetail);
	}
	fflush(stdout);

	if(snd_mixer_selem_set_playback_volume(elem,0,current)<0){
		snd_mixer_close(handle);
		return;
	}
	if(snd_mixer_selem_set_playback_volume(elem,1,current)<0){
		snd_mixer_close(handle);
		return;
	}

	snd_mixer_close(handle);
}

void snd_clear(struct playerHandles *ph){
	snd_pcm_state_t st;
	int flag=1;
	while(flag==1){
		st=snd_pcm_state(ph->sndfd);
		switch(st){
			case SND_PCM_STATE_SETUP:
			case SND_PCM_STATE_XRUN:
			case SND_PCM_STATE_OPEN:
			case SND_PCM_STATE_PREPARED:usleep(50000);break; // TODO: fix possible infinite loop
			case SND_PCM_STATE_DRAINING:
			case SND_PCM_STATE_RUNNING:
			default:flag=0;break;
		}
	}
	snd_pcm_drop(ph->sndfd);
	snd_pcm_prepare(ph->sndfd);
}

int writei_snd(struct playerHandles *ph, const char *out, const unsigned int size){
	int ret;
	if(ph->pflag->pause){
		snd_pcm_drop(ph->sndfd);
		do{
			usleep(100000); // 0.1 seconds
		}
		while(ph->pflag->pause);
		snd_pcm_prepare(ph->sndfd);
	}
	if(size==0){
		snd_pcm_drain(ph->sndfd);
		return 0;
	}
	ret=snd_pcm_writei(ph->sndfd,out,size);
	if(ret == -EAGAIN)return 0;
	if(ret<0){
		if(ret == -EPIPE){
			ret=snd_pcm_prepare(ph->sndfd);
			if(ret<0){
				snd_pcm_drain(ph->sndfd);
				snd_pcm_close(ph->sndfd);
				return -1;
			}
		}
		else if(ret == -ESTRPIPE){
			while((ret=snd_pcm_resume(ph->sndfd)) == -EAGAIN)
				sleep(1);
			if(ret<0){
				ret=snd_pcm_prepare(ph->sndfd);
				if(ret<0){
					snd_pcm_drain(ph->sndfd);
					snd_pcm_close(ph->sndfd);
					return -1;
				}
			}
		}
	}
	return 0;
}

void snd_close(struct playerHandles *ph){
	snd_pcm_close(ph->sndfd);
}

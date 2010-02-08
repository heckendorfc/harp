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
	snd_ctl_t *ctl;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *value;
	int err,current;

	if((err=snd_ctl_open(&ctl,ph->device,0))<0){
		return;
	}
	snd_ctl_elem_id_malloc(&id);
	snd_ctl_elem_value_malloc(&value);
	if(!id || !value){
		fprintf(stderr,"Malloc failed");
		return;
	}
	snd_ctl_elem_id_set_interface(id,SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id,"PCM Playback Volume");

	snd_ctl_elem_value_set_id(value,id);

	snd_ctl_elem_read(ctl,value);

	for(err=9;err>=0;err--){
		current=mod+snd_ctl_elem_value_get_integer(value,err);
		if(current<0)current=0;
		else if(current>100)current=100;
		snd_ctl_elem_value_set_integer(value,err,current);
	}
	fprintf(stdout,"\r                               Volume: %d%%  ",current);
	fflush(stdout);

	snd_ctl_elem_write(ctl,value);

	snd_ctl_elem_id_free(id);
	snd_ctl_elem_value_free(value);
	snd_ctl_close(ctl);
}

void toggleMute(struct playerHandles *ph, int *mute){
	snd_ctl_t *ctl;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *value;
	int current=*mute,err;

	if((err=snd_ctl_open(&ctl,ph->device,0))<0){
		return;
	}
	snd_ctl_elem_id_malloc(&id);
	snd_ctl_elem_value_malloc(&value);
	if(!id || !value){
		fprintf(stderr,"Malloc failed");
		return;
	}
	snd_ctl_elem_id_set_interface(id,SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id,"PCM Playback Volume");

	snd_ctl_elem_value_set_id(value,id);

	snd_ctl_elem_read(ctl,value);

	if(*mute>0){ // Unmute and perform volume change
		*mute=0;
		fprintf(stdout,"\r                               Volume: %d%%  ",current);
	}
	else{ // Mute 
		*mute=snd_ctl_elem_value_get_integer(value,0);
		fprintf(stdout,"\r                               Volume Muted  ");
	}
	fflush(stdout);

	for(err=9;err>=0;err--){
		snd_ctl_elem_value_set_integer(value,err,current);
	}


	snd_ctl_elem_write(ctl,value);
	snd_ctl_elem_id_free(id);
	snd_ctl_elem_value_free(value);
	snd_ctl_close(ctl);
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

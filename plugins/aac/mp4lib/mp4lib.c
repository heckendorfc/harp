/*
 *  Copyright (C) 2013-2015  Christian Heckendorf <heckendorfc@gmail.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "mp4lib.h"

#define checked_fread(a,b,c,d) if(fread(a,b,c,d)<c)return -1;
typedef int (*atom_f)(FILE*,mp4atom_t*,mp4handle_t*);

uint32_t parse_atom(FILE *in, mp4atom_t *at, mp4handle_t *h);

int namecmp(const char *a, const char *b){
	int i;
	for(i=0;i<4;i++)
		if(a[i]!=b[i])
			return 1;
	return 0;
}

int skip_stream(FILE *in, uint32_t len){
	const int blen=128;
	char buf[blen];
	uint32_t toread;
	int ret;

	while(len>0){
		toread=len>blen?blen:len;
		ret=fread(buf,1,toread,in);
		if(ret>0)
			len-=ret;
		else
			return -1;
	}

	return 0;
}

uint32_t swap_endianness(uint32_t in){
	return (in<<24) |
		((in&0xff00)<<8) |
		((in>>8)&0xff00) |
		(in>>24);
}

uint16_t swap_endianness16(uint16_t in){
	return (in<<8) | (in>>8);
}

int atom_step_down(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t cur_size;
	mp4atom_t cur_at;
	uint32_t ret;

	for(cur_size=0;cur_size<at->len;cur_size+=ret){
		ret=parse_atom(in,&cur_at,h);
		if(ret==0)return -1;
	}

	return 0;
}

int atom_moov_step_down(FILE *in, mp4atom_t *at, mp4handle_t *h){
	int ret = atom_step_down(in,at,h);
	h->metadone = 1;
	return ret;
}

int atom_trak_step_down(FILE *in, mp4atom_t *at, mp4handle_t *h){
	mp4tracklist_t *ptr=h->tracks;
	h->tracks=malloc(sizeof(*h->tracks));
	h->tracks->next=ptr;
	h->tracks->track.s_size=NULL;
	h->tracks->track.chunk_off=NULL;
	h->tracks->track.chunks=NULL;
	h->tracks->track.descs=NULL;
	return atom_step_down(in,at,h);
}

int atom_udta_start_down(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t cur_size;
	mp4atom_t cur_at;
	uint32_t ret;

	h->in_udta=1;
	h->metaptr=NULL;
	for(cur_size=0;cur_size<at->len;cur_size+=ret){
		ret=parse_atom(in,&cur_at,h);
		if(ret==0)return -1;
	}
	h->in_udta=0;

	return 0;
}

int atom_udta_meta_down(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t tmp;
	uint32_t cur_size;
	mp4atom_t cur_at;
	uint32_t ret;

	if(h->in_udta==0){
		skip_stream(in,at->len);
		return 0;
	}

	checked_fread(&tmp,1,4,in); // unknown

	for(cur_size=0;cur_size<at->len;cur_size+=ret){
		ret=parse_atom(in,&cur_at,h);
		if(ret==0)return -1;
	}

	return 0;
}

int atom_udta_ilst_down(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t cur_size;
	mp4atom_t cur_at;
	uint32_t ret;

	if(h->in_udta==0){
		return skip_stream(in,at->len);
	}

	for(cur_size=0;cur_size<at->len;cur_size+=ret){
		ret=parse_atom(in,&cur_at,h);
		if(ret==0)return -1;
	}

	return 0;
}

int atom_udta_parse_meta(FILE *in, mp4atom_t *at, mp4handle_t *h, char **ptr){
	uint32_t cur_size;
	mp4atom_t cur_at;
	uint32_t ret;

	if(h->in_udta==0){
		return skip_stream(in,at->len);
	}

	h->metaptr=*ptr=malloc(at->len+1);
	if(*ptr==NULL)
		return -1;
	bzero(*ptr,at->len+1);
	for(cur_size=0;cur_size<at->len;cur_size+=ret){
		ret=parse_atom(in,&cur_at,h);
		if(ret==0)return -1;
	}
	h->metaptr=NULL;

	return 0;
}

int atom_udta_parse_name(FILE *in, mp4atom_t *at, mp4handle_t *h){
	return atom_udta_parse_meta(in,at,h,&h->meta.title);
}

int atom_udta_parse_album(FILE *in, mp4atom_t *at, mp4handle_t *h){
	return atom_udta_parse_meta(in,at,h,&h->meta.album);
}

int atom_udta_parse_artist(FILE *in, mp4atom_t *at, mp4handle_t *h){
	return atom_udta_parse_meta(in,at,h,&h->meta.artist);
}

int atom_udta_parse_year(FILE *in, mp4atom_t *at, mp4handle_t *h){
	return atom_udta_parse_meta(in,at,h,&h->meta.year);
}

int atom_udta_parse_track(FILE *in, mp4atom_t *at, mp4handle_t *h){
	int ret;
	uint16_t *t;
	uint32_t size=at->len;
	ret=atom_udta_parse_meta(in,at,h,&h->meta.track); // TODO: Binary track to string?
	if(size>=8+4+4+4){
		t=((uint16_t*)h->meta.track)+1; // Skip leader
		*t=swap_endianness16(*t);
		sprintf(h->meta.track,"%d",*t);
	}
	return ret;
}

int atom_udta_parse_data(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t tmp;

	if(h->in_udta==0 || !h->metaptr){
		return skip_stream(in,at->len);
	}

	checked_fread(&tmp,4,1,in); // version
	checked_fread(&tmp,4,1,in); // null

	checked_fread(h->metaptr,1,at->len-8,in);

	return 0;
}

int atom_parse_mp4a(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t tmp;
	int i;
	mp4atom_t cur_at;

	for(i=0;i<6;i++)
		checked_fread(&tmp,1,1,in); // reserved
	checked_fread(&tmp,2,1,in); // data reference index
	checked_fread(&tmp,4,1,in); // reserved
	checked_fread(&tmp,4,1,in); // reserved

	checked_fread(&h->tracks->track.descs[h->tracks->track.num_desc].channels,2,1,in);
	h->tracks->track.descs[h->tracks->track.num_desc].channels=swap_endianness(h->tracks->track.descs[h->tracks->track.num_desc].channels);
	checked_fread(&h->tracks->track.descs[h->tracks->track.num_desc].samplesize,2,1,in);
	h->tracks->track.descs[h->tracks->track.num_desc].samplesize=swap_endianness(h->tracks->track.descs[h->tracks->track.num_desc].samplesize);

	checked_fread(&tmp,4,1,in); // skip

	checked_fread(&h->tracks->track.descs[h->tracks->track.num_desc].samplerate,2,1,in);
	h->tracks->track.descs[h->tracks->track.num_desc].samplerate=swap_endianness(h->tracks->track.descs[h->tracks->track.num_desc].samplerate);

	checked_fread(&tmp,2,1,in); // skip

	parse_atom(in,&cur_at,h);

	h->tracks->track.num_desc++;

	return 0;
}

int atom_parse_stco(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t i;
	uint32_t num;

	checked_fread(&i,4,1,in); // version and flags?
	checked_fread(&num,4,1,in); // num chunk
	num=swap_endianness(num);

	h->tracks->track.num_chunk=num;

	h->tracks->track.chunk_off=malloc(sizeof(*h->tracks->track.chunk_off)*num);
	if(h->tracks->track.chunk_off==NULL)
		return -1;
	for(i=0;i<num;i++){
		checked_fread(h->tracks->track.chunk_off+i,4,1,in);
		h->tracks->track.chunk_off[i]=swap_endianness(h->tracks->track.chunk_off[i]);
	}

	return 0;
}

int atom_parse_stsd(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t i;
	uint32_t num;
	mp4atom_t cur_at;

	checked_fread(&i,4,1,in); // version and flags
	checked_fread(&num,4,1,in); // num desc
	num=swap_endianness(num);

	h->tracks->track.num_desc=0;
	h->tracks->track.descs=malloc(sizeof(*h->tracks->track.descs)*num);
	if(h->tracks->track.descs==NULL)
		return -1;
	for(i=0;i<num;i++){
		parse_atom(in,&cur_at,h);
		if(namecmp(cur_at.name,"mp4a")!=0){
			h->tracks->track.descs[i].esds_len=0;
		}
		else{
			h->cur_track=h->tracks;
		}
		h->tracks->track.num_desc++;
	}
	h->tracks->track.num_desc=num;

	return 0;
}

/* Sample Size Box */
int atom_parse_stsz(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t i;

	checked_fread(&i,4,1,in); // version and flags
	checked_fread(&i,4,1,in); // sample size?

	checked_fread(&h->tracks->track.num_samples,4,1,in);
	h->tracks->track.num_samples=swap_endianness(h->tracks->track.num_samples);

	h->tracks->track.s_size=malloc(sizeof(*h->tracks->track.s_size)*h->tracks->track.num_samples);
	if(h->tracks->track.s_size==NULL)
		return -1;

	for(i=0;i<h->tracks->track.num_samples;i++){
		checked_fread(h->tracks->track.s_size+i,4,1,in);
		h->tracks->track.s_size[i]=swap_endianness(h->tracks->track.s_size[i]);
	}

	return 0;
}

/* Sample to Chunk Box */
int atom_parse_stsc(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t i;

	checked_fread(&i,4,1,in); // version and flags

	/* TODO: Check if ~0 */
	checked_fread(&h->tracks->track.num_map_chunk,4,1,in);
	h->tracks->track.num_map_chunk=swap_endianness(h->tracks->track.num_map_chunk);

	h->tracks->track.chunks=malloc(sizeof(*h->tracks->track.chunks)*h->tracks->track.num_map_chunk);
	if(h->tracks->track.chunks==NULL)
		return -1;

	for(i=0;i<h->tracks->track.num_map_chunk;i++){
		checked_fread(&h->tracks->track.chunks[i].start,4,1,in);
		checked_fread(&h->tracks->track.chunks[i].num_sample,4,1,in);
		checked_fread(&h->tracks->track.chunks[i].desc_index,4,1,in);

		h->tracks->track.chunks[i].start=swap_endianness(h->tracks->track.chunks[i].start);
		h->tracks->track.chunks[i].num_sample=swap_endianness(h->tracks->track.chunks[i].num_sample);
		h->tracks->track.chunks[i].desc_index=swap_endianness(h->tracks->track.chunks[i].desc_index);
	}

	return 0;
}

uint32_t descr_length(FILE *in,int *num){
	uint32_t ret=0;
	uint8_t byte;

	*num=0;
	do{
		checked_fread(&byte,1,1,in);
		(*num)++;
		ret=(ret<<7)|(byte&0x7F);
	}while(byte&0x80 && *num<4);

	return ret;
}

/* ES Descriptor */
int atom_parse_esds(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t i;
	uint8_t tch;
	uint32_t size=at->len;
	int count;

	checked_fread(&i,4,1,in); // version and flags
	size-=4;

	checked_fread(&tch,1,1,in);
	size--;
	if(tch==0x03){
		i=descr_length(in,&count);
		if(i<5+15){
			goto fail;
		}
		checked_fread(&i,3,1,in); // skip
		size-=count+3;
	}
	else{
		checked_fread(&i,2,1,in); // skip
		size-=2;
	}

	checked_fread(&tch,1,1,in);
	size--;
	if(tch!=0x04){
		goto fail;
	}

	/* TODO: Check if ~0 */
	i=descr_length(in,&count);
	size-=count;
	if(i<13){
		goto fail;
	}

	checked_fread(&h->tracks->track.descs[h->tracks->track.num_desc].type,1,1,in);

	checked_fread(&i,4,1,in);

	checked_fread(&h->tracks->track.descs[h->tracks->track.num_desc].maxbitrate,4,1,in);
	h->tracks->track.descs[h->tracks->track.num_desc].maxbitrate=swap_endianness(h->tracks->track.descs[h->tracks->track.num_desc].maxbitrate);

	checked_fread(&h->tracks->track.descs[h->tracks->track.num_desc].avgbitrate,4,1,in);
	h->tracks->track.descs[h->tracks->track.num_desc].avgbitrate=swap_endianness(h->tracks->track.descs[h->tracks->track.num_desc].avgbitrate);

	size-=13;

	checked_fread(&tch,1,1,in);
	size--;
	if(tch!=0x05)
		goto fail;

	h->tracks->track.descs[h->tracks->track.num_desc].esds_len=descr_length(in,&count);
	size-=count;
	h->tracks->track.descs[h->tracks->track.num_desc].esds=malloc(sizeof(h->tracks->track.descs[h->tracks->track.num_desc].esds)*h->tracks->track.descs[h->tracks->track.num_desc].esds_len);
	if(h->tracks->track.descs[h->tracks->track.num_desc].esds==NULL)
		return -1;
	checked_fread(h->tracks->track.descs[h->tracks->track.num_desc].esds,1,h->tracks->track.descs[h->tracks->track.num_desc].esds_len,in);
	size-=h->tracks->track.descs[h->tracks->track.num_desc].esds_len;

fail:
	for(;size>0;size--)
		checked_fread(&tch,1,1,in);
	return 0;
}

struct atom_list{
	char name[4];
	atom_f f;
} atoms[]={
	{"moov",atom_moov_step_down},
	 {"trak",atom_trak_step_down},
	  {"mdia",atom_step_down},
	   {"minf",atom_step_down},
	    {"stbl",atom_step_down},
	     {"stco",atom_parse_stco},
	     {"stsc",atom_parse_stsc},
	     {"stsz",atom_parse_stsz},
	     {"stsd",atom_parse_stsd},
	      {"mp4a",atom_parse_mp4a},
	       {"esds",atom_parse_esds},
	 {"udta",atom_udta_start_down},
	  {"meta",atom_udta_meta_down},
	   {"ilst",atom_udta_ilst_down},
	    {"\xa9""nam",atom_udta_parse_name},
	    {"\xa9""alb",atom_udta_parse_album},
	    {"\xa9""ART",atom_udta_parse_artist},
	    {"\xa9""day",atom_udta_parse_year},
	    {"trkn",atom_udta_parse_track},
	     {"data",atom_udta_parse_data},
	//{"mdat",NULL},
	{"\0\0\0\0",NULL}
};

uint32_t parse_atom(FILE *in, mp4atom_t *at, mp4handle_t *h){
	int found=0;
	uint32_t a_len;
	struct atom_list *al_item;

	if(fread(&a_len,4,1,in)<1)
		return 0;
	if(fread(at->name,1,4,in)<1)
		return 0;

	a_len=swap_endianness(a_len);
	a_len-=8;

	at->len=a_len;
	for(al_item=atoms;*al_item->name!=0;al_item++){
		if(namecmp(al_item->name,at->name)==0){
			if(al_item->f==NULL){
				h->metadone=1;
				return 0;
			}
			found=1;
			if(al_item->f(in,at,h)<0)
				return 0;
			break;
		}
	}

	if(!found){
		if(skip_stream(in,a_len)<0)
			return 0;
	}

	return a_len+8;
}

int mp4lib_open(mp4handle_t *h){
	bzero(h,sizeof(*h));
	h->next_sample=0;
	h->s_buf.buf=malloc(1024);
	if(h->s_buf.buf==NULL)
		return -1;
	h->s_buf.allocated=1024;
	h->in_udta=0;
	h->metadone=0;
	h->cur_track=NULL;
	h->tracks=NULL;
	//bzero(&h->meta,sizeof(h->meta));
	return 0;
}

static void get_best_track(mp4handle_t *h){
	if(!h->cur_track)
		h->cur_track=h->tracks;
}

int mp4lib_parse_meta(FILE *in, mp4handle_t *h){
	mp4atom_t at;
	while(parse_atom(in,&at,h)>0 && !h->metadone);
	get_best_track(h);
	if(h->metadone && h->cur_track->track.chunk_off)
		return fseek(in,h->cur_track->track.chunk_off[0],SEEK_SET);
	return !h->metadone;
}

int mp4lib_get_decoder_config(mp4handle_t *h, int track, unsigned char **buf, unsigned int *size){
	if(track>=h->cur_track->track.num_desc)
		return -1;

	*buf=h->cur_track->track.descs[track].esds;
	*size=h->cur_track->track.descs[track].esds_len;
	return 0;
}

int mp4lib_total_tracks(mp4handle_t *h){
	return h->cur_track->track.num_desc;
}

int mp4lib_num_samples(mp4handle_t *h){
	return h->cur_track->track.num_samples;
}

long sum_sample_sizes(mp4handle_t *h, int start, int len){
	long size;
	int i;

	size=0;
	for(i=0;i<len;i++){
		size+=h->cur_track->track.s_size[start+i];
	}

	return size;
}

int seek_to(FILE *in, long offset){
	return fseek(in,offset,SEEK_SET);
}

int seek_back(FILE *in, mp4handle_t *h, int skipto){
	long size=sum_sample_sizes(h,skipto,h->next_sample-skipto);
	if(seek_to(in,-size)<0)
		return 1;
	h->next_sample=skipto;
	return 0;
}

int seek(FILE *in, mp4handle_t *h, int skipto){
	int ns=h->cur_track->track.chunks[0].num_sample;
	int chunk=skipto/ns;
	int extra=skipto%ns;
	long size=sum_sample_sizes(h,chunk*ns,extra);
	if(seek_to(in,h->cur_track->track.chunk_off[chunk]+size)<0)
		return 1;
	h->next_sample=skipto;
	return 0;
}

int mp4lib_read_sample(FILE *in, mp4handle_t *h, int sample, unsigned char **buf, unsigned int *size){
	if(sample<h->next_sample){
		if(sample<0)
			sample=0;
		if(seek(in,h,sample))
			return -1;
	}
	else if(sample>h->next_sample){
		if(sample>h->cur_track->track.num_samples)
			return 0;
		if(seek(in,h,sample))
			return -1;
	}

	*size=h->cur_track->track.s_size[sample];
	if(*size>h->s_buf.allocated){
		h->s_buf.buf=malloc(*size);
		if(h->s_buf.buf==NULL)
			return -1;
		h->s_buf.allocated=*size;
	}
	*buf=h->s_buf.buf;
	checked_fread(*buf,1,h->cur_track->track.s_size[sample],in);

	h->next_sample++;

	if((h->next_sample%h->cur_track->track.chunks[0].num_sample)==0){
		int chunk=h->next_sample/h->cur_track->track.chunks[0].num_sample;
		if(seek_to(in,h->cur_track->track.chunk_off[chunk]))
			return -1;
	}

	return 0;
}

int mp4lib_prime_read(FILE *in, mp4handle_t *h){
	if(seek_to(in,h->cur_track->track.chunk_off[0])<0)
		return 1;
	return 0;
}

void mp4lib_close(mp4handle_t *h){
	mp4tracklist_t *ptr=h->tracks;
	mp4tracklist_t *tmp;

	while(ptr){
		if(ptr->track.s_size)
			free(ptr->track.s_size);
		if(ptr->track.chunks)
			free(ptr->track.chunks);
		if(ptr->track.chunk_off)
			free(ptr->track.chunk_off);
		if(ptr->track.descs)
			free(ptr->track.descs);
		tmp=ptr->next;
		free(ptr);
		ptr=tmp;
	}

	if(h->s_buf.buf)
		free(h->s_buf.buf);
	if(h->meta.title)
		free(h->meta.title);
	if(h->meta.album)
		free(h->meta.album);
	if(h->meta.artist)
		free(h->meta.artist);
	if(h->meta.year)
		free(h->meta.year);
	if(h->meta.track)
		free(h->meta.track);
}

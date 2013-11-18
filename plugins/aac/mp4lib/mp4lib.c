#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
//#include <string.h>

#include "mp4lib.h"

typedef void (*atom_f)(FILE*,mp4atom_t*,mp4handle_t*);

uint32_t parse_atom(FILE *in, mp4atom_t *at, mp4handle_t *h);

int namecmp(const char *a, const char *b){
	int i;
	for(i=0;i<4;i++)
		if(a[i]!=b[i])
			return 1;
	return 0;
}

void skip_stream(FILE *in, uint32_t len){
	const int blen=128;
	char buf[blen];
	uint32_t toread;
	int ret;

	while(len>0){
		toread=len>blen?blen:len;
		ret=fread(buf,1,toread,in);
		if(ret>0)
			len-=ret;
	}
}

int init_atom(mp4atom_t *at){
	at->len=0;
	at->allocated=INIT_BUF_SIZE;
	at->buf=malloc(INIT_BUF_SIZE);

	return 0;
}

static int resize_atom(mp4atom_t *at, uint32_t len){
	if(len<at->allocated)
		return 0;

	at->buf=realloc(at->buf,len);
	at->allocated=len;

	return 0;
}

uint32_t swap_endianness(uint32_t in){
	return (in<<24) |
		((in&0xff00)<<8) |
		((in>>8)&0xff00) |
		(in>>24);
}

void atom_step_down(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t cur_size;
	mp4atom_t cur_at;
	uint32_t ret;

	for(cur_size=0;cur_size<at->len;cur_size+=ret){
		ret=parse_atom(in,&cur_at,h);
		if(ret==0)return;
	}
}

void atom_parse_mp4a(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t tmp;
	int i;
	mp4atom_t cur_at;

	for(i=0;i<6;i++)
		fread(&tmp,1,1,in); // reserved
	fread(&tmp,2,1,in); // data reference index
	fread(&tmp,4,1,in); // reserved
	fread(&tmp,4,1,in); // reserved

	fread(&h->descs[h->num_desc].channels,2,1,in);
	h->descs[h->num_desc].channels=swap_endianness(h->descs[h->num_desc].channels);
	fread(&h->descs[h->num_desc].samplesize,2,1,in);
	h->descs[h->num_desc].samplesize=swap_endianness(h->descs[h->num_desc].samplesize);

	fread(&tmp,4,1,in); // skip

	fread(&h->descs[h->num_desc].samplerate,2,1,in);
	h->descs[h->num_desc].samplerate=swap_endianness(h->descs[h->num_desc].samplerate);

	fread(&tmp,2,1,in); // skip

	parse_atom(in,&cur_at,h);

	h->num_desc++;
}

void atom_parse_stsd(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t i;
	uint32_t num;
	mp4atom_t cur_at;

	fread(&i,4,1,in); // version and flags
	fread(&num,4,1,in); // num desc
	num=swap_endianness(num);

	/*
	fread(&h->num_samples,4,1,in);
	h->num_samples=swap_endianness(h->num_samples);

	h->s_size=malloc(sizeof(*h->s_size)*h->num_samples);
	*/

	h->num_desc=0;
	h->descs=malloc(sizeof(*h->descs)*num);
	for(i=0;i<num;i++){
		parse_atom(in,&cur_at,h);
		if(namecmp(cur_at.name,"mp4a")!=0){
			h->descs[i].esds_len=0;
		}
		h->num_desc++;
	}
	h->num_desc=num;
}

/* Sample Size Box */
void atom_parse_stsz(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t i;

	fread(&i,4,1,in); // version and flags
	fread(&i,4,1,in); // sample size?

	fread(&h->num_samples,4,1,in);
	h->num_samples=swap_endianness(h->num_samples);

	h->s_size=malloc(sizeof(*h->s_size)*h->num_samples);

	for(i=0;i<h->num_samples;i++){
		fread(h->s_size+i,4,1,in);
		h->s_size[i]=swap_endianness(h->s_size[i]);
	}
}

/* Sample to Chunk Box */
void atom_parse_stsc(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t i;

	fread(&i,4,1,in); // version and flags

	/* TODO: Check if ~0 */
	fread(&h->num_chunk,4,1,in);
	h->num_chunk=swap_endianness(h->num_chunk);

	h->chunks=malloc(sizeof(*h->chunks)*h->num_chunk);

	for(i=0;i<h->num_chunk;i++){
		fread(&h->chunks[i].start,4,1,in);
		fread(&h->chunks[i].num_sample,4,1,in);
		fread(&h->chunks[i].desc_index,4,1,in);

		h->chunks[i].start=swap_endianness(h->chunks[i].start);
		h->chunks[i].num_sample=swap_endianness(h->chunks[i].num_sample);
		h->chunks[i].desc_index=swap_endianness(h->chunks[i].desc_index);
	}
}

uint32_t descr_length(FILE *in,int *num){
	uint32_t ret=0;
	uint8_t byte;

	*num=0;
	do{
		fread(&byte,1,1,in);
		(*num)++;
		ret=(ret<<7)|(byte&0x7F);
	}while(byte&0x80 && *num<4);

	return ret;
}

/* ES Descriptor */
void atom_parse_esds(FILE *in, mp4atom_t *at, mp4handle_t *h){
	uint32_t i;
	uint8_t tch;
	uint32_t size=at->len;
	int count;

	fread(&i,4,1,in); // version and flags
	size-=4;

	fread(&tch,1,1,in);
	size--;
	if(tch==0x03){
		i=descr_length(in,&count);
		if(i<5+15){
			goto fail;
		}
		fread(&i,3,1,in); // skip
		size-=count+3;
	}
	else{
		fread(&i,2,1,in); // skip
		size-=2;
	}

	fread(&tch,1,1,in);
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

	fread(&h->descs[h->num_desc].type,1,1,in);

	fread(&i,4,1,in);

	fread(&h->descs[h->num_desc].maxbitrate,4,1,in);
	h->descs[h->num_desc].maxbitrate=swap_endianness(h->descs[h->num_desc].maxbitrate);

	fread(&h->descs[h->num_desc].avgbitrate,4,1,in);
	h->descs[h->num_desc].avgbitrate=swap_endianness(h->descs[h->num_desc].avgbitrate);

	size-=13;

	fread(&tch,1,1,in);
	size--;
	if(tch!=0x05)
		goto fail;

	h->descs[h->num_desc].esds_len=descr_length(in,&count);
	size-=count;
	h->descs[h->num_desc].esds=malloc(sizeof(h->descs[h->num_desc].esds)*h->descs[h->num_desc].esds_len);
	fread(h->descs[h->num_desc].esds,1,h->descs[h->num_desc].esds_len,in);
	size-=h->descs[h->num_desc].esds_len;

fail:
	for(;size>0;size--)
		fread(&tch,1,1,in);
	return;
}

struct atom_list{
	char name[4];
	atom_f f;
} atoms[]={
	{"moov",atom_step_down},
	 {"trak",atom_step_down},
	  {"mdia",atom_step_down},
	   {"minf",atom_step_down},
	    {"stbl",atom_step_down},
	     {"stsc",atom_parse_stsc},
	     {"stsz",atom_parse_stsz},
	     {"stsd",atom_parse_stsd},
	      {"mp4a",atom_parse_mp4a},
	       {"esds",atom_parse_esds},
	{"mdat",NULL},
	{"\0\0\0\0",NULL}
};

uint32_t parse_atom(FILE *in, mp4atom_t *at, mp4handle_t *h){
	int found=0;
	uint32_t a_len;
	struct atom_list *al_item;

	fread(&a_len,4,1,in);
	fread(at->name,1,4,in);

	a_len=swap_endianness(a_len);
	a_len-=8;

	at->len=a_len;

	for(al_item=atoms;*al_item->name!=0;al_item++){
		if(namecmp(al_item->name,at->name)==0){
			if(al_item->f==NULL)
				return 0;
			found=1;
			al_item->f(in,at,h);
			break;
		}
	}

	if(!found){
		skip_stream(in,a_len);
	}

	/*
	resize_atom(at,a_len);
	at->len=a_len;

	fread(at->buf,1,a_len,in);
	*/

	return a_len+8;
}

void mp4lib_open(mp4handle_t *h){
	h->next_sample=0;
	h->s_buf.buf=malloc(1024);
	h->s_buf.allocated=1024;
}

void mp4lib_parse_meta(FILE *in, mp4handle_t *h){
	mp4atom_t at;
	//init_atom(&at);
	while(parse_atom(in,&at,h)>0);
}

int mp4lib_get_decoder_config(mp4handle_t *h, int track, unsigned char **buf, unsigned int *size){
	if(track>=h->num_desc)
		return -1;

	*buf=h->descs[track].esds;
	*size=h->descs[track].esds_len;
	return 0;
}

int mp4lib_total_tracks(mp4handle_t *h){
	return h->num_desc;
}

int mp4lib_num_samples(mp4handle_t *h){
	return h->num_samples;
}

long sum_sample_sizes(mp4handle_t *h, int start, int len){
	long size;
	int i;

	size=0;
	for(i=0;i<len;i++){
		size+=h->s_size[start+i];
	}

	return size;
}

void seek_to(FILE *in, long offset){
	long pos=ftell(in);
	fseek(in,pos+offset,SEEK_SET);
}

void seek_back(FILE *in, mp4handle_t *h, int skipto){
	long size=sum_sample_sizes(h,skipto,h->next_sample-skipto);
	seek_to(in,-size);
	h->next_sample=skipto;
}

void seek_forward(FILE *in, mp4handle_t *h, int skipto){
	long size=sum_sample_sizes(h,h->next_sample,skipto-h->next_sample);
	seek_to(in,size);
	h->next_sample=skipto;
}

int mp4lib_read_sample(FILE *in, mp4handle_t *h, int sample, unsigned char **buf, unsigned int *size){
	if(sample<h->next_sample)
		seek_back(in,h,sample);
	else if(sample>h->next_sample)
		seek_forward(in,h,sample);

	*size=h->s_size[sample];
	if(*size>h->s_buf.allocated){
		h->s_buf.buf=malloc(*size);
		h->s_buf.allocated=*size;
	}
	*buf=h->s_buf.buf;
	fread(*buf,1,h->s_size[sample],in);

	h->next_sample++;

	return 0;
}

void mp4lib_close(mp4handle_t *h){
	free(h->s_size);
	free(h->chunks);
	free(h->descs);
	free(h->s_buf.buf);
}

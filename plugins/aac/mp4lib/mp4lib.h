/*
 *  Copyright (C) 2014  Christian Heckendorf <heckendorfc@gmail.com>
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

#ifndef MP4LIB_H
#define MP4LIB_H

#include <stdio.h>
#include <stdint.h>

#define INIT_BUF_SIZE 512

typedef struct mp4track_t{
	uint32_t *s_size;
	uint32_t num_samples;

	uint32_t *chunk_off;
	uint32_t num_chunk;

	struct chunk_map{
		uint32_t start;
		uint32_t desc_index;
		uint32_t num_sample;
	}*chunks;
	uint32_t num_map_chunk;

	struct samp_desc{
		uint16_t channels;
		uint16_t samplesize;
		uint16_t samplerate;
		uint32_t avgbitrate;
		uint32_t maxbitrate;
		uint8_t type;

		unsigned char *esds;
		uint32_t esds_len;
	}*descs;
	uint32_t num_desc;

}mp4track_t;

typedef struct mp4tracklist_t{
	struct mp4tracklist_t *next;
	struct mp4track_t track;
}mp4tracklist_t;

typedef struct mp4handle_t{
	mp4tracklist_t *tracks;

	struct udta{
		char *title;
		char *album;
		char *artist;
		char *year;
		char *track;
	}meta;
	int in_udta;
	char *metaptr;

	struct sample_buf{
		unsigned char *buf;
		int allocated;
	}s_buf;

	mp4tracklist_t *cur_track;
	uint8_t metadone;
	uint32_t next_sample;
}mp4handle_t;

typedef struct mp4atom_t{
	char name[4];
	uint8_t *buf;
	uint32_t len;
	uint32_t allocated;
}mp4atom_t;

int mp4lib_open(mp4handle_t *h);
int init_atom(mp4atom_t *at);
int mp4lib_parse_meta(FILE *in, mp4handle_t *h);
int mp4lib_get_decoder_config(mp4handle_t *h, int track, unsigned char **buf, unsigned int *size);
int mp4lib_total_tracks(mp4handle_t *h);
int mp4lib_read_sample(FILE *in, mp4handle_t *h, int sample, unsigned char **buf, unsigned int *size);
int mp4lib_prime_read(FILE *in, mp4handle_t *h);
int mp4lib_num_samples(mp4handle_t *h);
void mp4lib_close(mp4handle_t *h);

#endif

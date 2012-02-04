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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

void plugin_meta(FILE *ffd, struct musicInfo *mi){
	return;
	/*
	struct snd_data data;
	FLAC__StreamDecoder *decoder=NULL;
	if((decoder=FLAC__stream_decoder_new())==NULL){
		fprintf(stderr,"flac decoder alloc failed");
		return;
	}

	FLAC__stream_decoder_set_metadata_ignore_all(decoder);
	FLAC__stream_decoder_set_metadata_respond(decoder,FLAC__METADATA_TYPE_STREAMINFO);

	if(FLAC__stream_decoder_init_FILE(decoder,ph->ffd,flac_write,flac_meta,flac_error,&data)!=FLAC__STREAM_DECODER_INIT_STATUS_OK){
		fprintf(stderr,"flac init failed");
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
		return;
	}

	if(!FLAC__stream_decoder_process_until_end_of_metadata(decoder)){
		fprintf(stderr,"flac decoder metadata failed");
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
		return DEC_RET_ERROR;
	}

	mi->length=data.rate;

	return;
	*/
}

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

struct musicInfo* plugin_meta(FILE *ffd){
	struct musicInfo *mi=calloc(1,sizeof(struct musicInfo));
	mp4ff_t infile;
	mp4ff_callback_t *mp4cb = malloc(sizeof(mp4ff_callback_t));
	mp4cb->read=read_callback;
	mp4cb->seek=seek_callback;
	mp4cb->user_data=ffd;

	infile=mp4ff_open_read(mp4cb);
	if(!infile){
		fprintf(stderr,"mp4ffopenread failed");
		free(mp4cb);
		return mi;
	}

	char *temp;
	if(mp4ff_meta_get_title(infile,&temp)){
		mi->title=malloc(sizeof(char)*61);
		strncpy(mi->title,temp,60);
		free(temp);
	}

	if(mp4ff_meta_get_artist(infile,&temp)){
		mi->artist=malloc(sizeof(char)*61);
		strncpy(mi->artist,temp,60);
		free(temp);
	}

	if(mp4ff_meta_get_album(infile,&temp)){
		mi->album=malloc(sizeof(char)*61);
		strncpy(mi->album,temp,60);
		free(temp);
	}

	if(mp4ff_meta_get_track(infile,&temp)){
		mi->track=malloc(sizeof(char)*61);
		strncpy(mi->track,temp,60);
		free(temp);
	}

	if(mp4ff_meta_get_date(infile,&temp)){
		mi->year=malloc(sizeof(char)*9);
		strncpy(mi->year,temp,8);
		free(temp);
	}

	printf("%s | %s | %s | %s| %s\n\n",mi->title,mi->track,mi->album,mi->artist,mi->year);

	free(mp4cb);
	return mi;
}

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

void plugin_meta(FILE *ffd, struct musicInfo *mi){
	mp4handle_t handle;

	mi->length=-1;

	if(mp4lib_open(&handle))
		return;

	mp4lib_parse_meta(ffd,&handle);

	if(handle.meta.title){
		strncpy(mi->title,handle.meta.title,60);
	}

	if(handle.meta.artist){
		strncpy(mi->artist,handle.meta.artist,60);
	}

	if(handle.meta.album){
		strncpy(mi->album,handle.meta.album,60);
	}

	if(handle.meta.track){
		strncpy(mi->track,handle.meta.track,8);
	}

	if(handle.meta.year){
		strncpy(mi->year,handle.meta.year,8);
	}

	mp4lib_close(&handle);
}

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
	OggVorbis_File *vf;
	if(!(vf=malloc(sizeof(OggVorbis_File)))){
		fprintf(stderr,"Malloc failed (vf).");
		return;
	}

	if(ov_open_callbacks(ffd,vf,NULL,0,OV_CALLBACKS_NOCLOSE)<0){
		fprintf(stderr,"ov open failed\n");
		free(vf);
		return;
	}

	if((mi->length=ov_time_total(vf,-1))<1)
		mi->length=-1;
		printf("%d\n",mi->length);

	ov_clear(vf);
	return;
}

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

/* ID3......TTAG.....valueTTG2......value */

static int intpow(const int base, int exp){
	int ret=base;
	if(!exp)return 1;
	while(--exp){
		ret*=base;
	}
	return ret;
}

int getTagData(unsigned char *buf, struct musicInfo *mi){
	int x=0,ret=0,y;
	unsigned char *tag=buf+4;
	unsigned char *temp=calloc(5,sizeof(char));
	
	for(x=4;*tag==0 && x>0;x--)tag++;
	memcpy(temp,tag,x); // Info Size
	tag+=(x+3);
	y=ret=x;

	//for(x=0;ret>0;ret--)x+=(int)pow((double)temp[y-ret],(double)ret); // Prep for strtol
	for(x=0;ret>0;ret--)x+=intpow(temp[y-ret],ret); // Should this be x+=temp[y-ret]*intpow(256,ret)? Not that it matters since max field size will be ~200.

	if(memcmp("TIT2",buf,4)==0){
		memcpy(mi->title,tag,(x-1)>MI_TITLE_SIZE?MI_TITLE_SIZE:x-1);
	}
	else if(memcmp("TRCK",buf,4)==0){
		memcpy(mi->track,tag,(x-1)>MI_TRACK_SIZE?MI_TRACK_SIZE:x-1);
	}
	else if(memcmp("TALB",buf,4)==0){
		memcpy(mi->album,tag,(x-1)>MI_ALBUM_SIZE?MI_ALBUM_SIZE:x-1);
	}
	else if(memcmp("TYER",buf,4)==0){
		memcpy(mi->year,tag,(x-1)>MI_YEAR_SIZE?MI_YEAR_SIZE:x-1);
	}
	//else if(memcmp("TLEN",buf,4)==0){
		//memcpy(mi->length,tag,(x-1)>MI_LENGTH_SIZE?MI_LENGTH_SIZE:x-1);
	//}
	else if(memcmp("TPE1",buf,4)==0){
		memcpy(mi->artist,tag,(x-1)>MI_ARTIST_SIZE?MI_ARTIST_SIZE:x-1);
	}
	else if(memcmp("\0\0\0\0",buf,4)==0)return -1;

	free(temp);
	return x+10;
}
	
void ID3v2Parse(FILE *ffd, struct musicInfo *mi){
	int ret,total=3000,next=0;
	unsigned char buffer[255];
	fseek(ffd,next,SEEK_SET);
	if(fread(buffer,sizeof(char),10,ffd)<10)return;

	next+=10;
	do{
		fseek(ffd,next,SEEK_SET);
		if(fread(buffer,sizeof(char),255,ffd)<255)next=total;
		ret=getTagData(buffer,mi);
		next+=ret;
	}while(ret>0 && next<total);
}

void ID3v1Parse(FILE *ffd, struct musicInfo *mi){
	int next=-125;
	char buffer[31],safe[61];
	fseek(ffd,next,SEEK_END);
	if(fread(mi->title,sizeof(char),30,ffd)<30)return;
	next+=30;

	fseek(ffd,next,SEEK_END);
	if(fread(mi->artist,sizeof(char),30,ffd)<30)return;
	next+=30;

	fseek(ffd,next,SEEK_END);
	if(fread(mi->album,sizeof(char),30,ffd)<30)return;
	next+=30;

	fseek(ffd,next,SEEK_END);
	if(fread(mi->year,sizeof(char),4,ffd)<4)return;
	next+=4;
}

int mp3Length(FILE *ffd, int quick){
	int len;
	long rate;
	mpg123_init();
	mpg123_handle *m;
	if(!(m = mpg123_new(NULL, NULL))){
		fprintf(stderr,"Unable to create mpg123 handle\n");
		return -1;
	}
	mpg123_param(m, MPG123_FLAGS, MPG123_QUIET, 0); 
	mpg123_open_fd(m,fileno(ffd));
	if(m == NULL) return -1;
	
	if(!quick)
		mpg123_scan(m);

	mpg123_getformat(m,&rate,NULL,NULL);

	if(rate && (len=mpg123_length(m))!=MPG123_ERR)
		len/=rate;
	else
		len=-1;

	mpg123_delete(m);
	mpg123_exit();
	return len;
}

void plugin_meta(FILE *ffd, struct musicInfo *mi){
	int MAX_VERSION=4;
	int next=0;
	unsigned char buffer[255];
	fseek(ffd,next,SEEK_SET);
	if(fread(buffer,sizeof(char),10,ffd)<10)return;
	if(memcmp("ID3",buffer,3)==0 && buffer[4]<=MAX_VERSION){ // ID3v2
		ID3v2Parse(ffd,mi);
	}
	else{
		fseek(ffd,-128,SEEK_END);
		if(fread(buffer,sizeof(char),4,ffd)<4)return;
		if(memcmp("TAG",buffer,3)==0)ID3v1Parse(ffd,mi); // ID3v1
	}

	mi->length=mp3Length(ffd,mi->length);
}

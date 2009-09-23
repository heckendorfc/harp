/*
 *  Copyright (C) 2009  Christian Heckendorf <heckendorfc@gmail.com>
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

#include <math.h>

void safe_store(char *str, char *data, size_t size){
	if(strcmp(data,"")==0){strcpy(str,"Unknown");return;}
	int x,z=0;
	for(x=0;data[x]>31 && data[x]<127 && x<size;x++){//strip multi space
		if(data[x]==' ' && data[x+1]==' ')continue;
		str[z]=data[x];
		z++;
	}
	str[z]=0;
	if(str[z-1]==' ')str[z-1]=0;
	for(x=0;str[x]!='\0';x++){//addslashes
		if(str[x]=='\''){
			memmove(&str[x+1],&str[x],(z+1)-x);
			str[x]='\'';
			x++;
			z++;
		}
	}
	//fprintf(stderr,"%s\n",str);
	//if(str[x-1]==' ')//strip trailing space
	//	str[x-1]=0;
	if(strcmp(str,"")==0)strcpy(str,"Unknown");
}

/* ID3......TTAG.....valueTTG2......value */
int getTagData(unsigned char *buf, struct musicInfo *mi){
	int x=0,ret=0,y;
	//for(;x<4;x++)printf("%c",buf[x]);printf("\n");x=0;
	//for(;x<4;x++)printf("%x",buf[x]);printf("\n");x=0;
	unsigned char *tag=buf+4;
	unsigned char *temp=calloc(5,sizeof(char));
	//tag+=4;ret+=4;
	for(x=4;*tag==0 && x>0;x--)tag++;
	memcpy(temp,tag,x); // Info Size
	tag+=(x+3);
	y=ret=x;

	for(x=0;ret>0;ret--)x+=(int)pow((double)temp[y-ret],(double)ret); // Prep for strtol
//	fprintf(stderr,"%x %d %d %x\n",*(tag-(3+y)),y,x,temp[0]);

	if(memcmp("TIT2",buf,4)==0){
		mi->title=calloc(x*2,sizeof(char));
		memcpy(mi->title,tag,x-1);
	}
	else if(memcmp("TRCK",buf,4)==0){
		mi->track=calloc(x*2,sizeof(char));
		memcpy(mi->track,tag,x-1);
	}
	else if(memcmp("TALB",buf,4)==0){
		mi->album=calloc(x*2,sizeof(char));
		memcpy(mi->album,tag,x-1);
	}
	else if(memcmp("TYER",buf,4)==0){
		mi->year=calloc(x*2,sizeof(char));
		memcpy(mi->year,tag,x-1);
	}
	else if(memcmp("TLEN",buf,4)==0){
		mi->length=calloc(x*2,sizeof(char));
		memcpy(mi->length,tag,x-1);
	}
	else if(memcmp("TPE1",buf,4)==0){
		mi->artist=calloc(x*2,sizeof(char));
		memcpy(mi->artist,tag,x-1);
	}
	else if(memcmp("\0\0\0\0",buf,4)==0)return -11;

	free(temp);
	return x+10;
	//free(loc);
	//while(--x)printf("%c",*tag++);
	//printf("\n");
}
	
void ID3v2Parse(FILE *ffd, struct musicInfo *mi){
	int ret,total=3000,next=0;
	unsigned char buffer[255];
	fseek(ffd,next,SEEK_SET);
	fread(buffer,sizeof(char),10,ffd);
	//for(ret=4;ret>0;ret--)
	//	total+=(buffer[10-ret]<<(8*(ret-1)));
	next+=10;
	do{
		fseek(ffd,next,SEEK_SET);
		fread(buffer,sizeof(char),255,ffd);
		ret=getTagData(buffer,mi);
		next+=ret;
	}while(ret>0 && next<total);
	printf("%s | %s | %s | %s| %s | %s || %d\n\n",mi->title,mi->track,mi->album,mi->artist,mi->length,mi->year,next);
}

void ID3v1Parse(FILE *ffd, struct musicInfo *mi){
	int next=-125;
	char buffer[31],safe[61];
	fseek(ffd,next,SEEK_END);
	fread(buffer,sizeof(char),30,ffd);
	safe_store(safe,buffer,30);
	mi->title=calloc(61,sizeof(char));
	strcpy(mi->title,safe);
	next+=30;

	fseek(ffd,next,SEEK_END);
	fread(buffer,sizeof(char),30,ffd);
	safe_store(safe,buffer,60);
	mi->artist=calloc(61,sizeof(char));
	strcpy(mi->artist,safe);
	next+=30;

	fseek(ffd,next,SEEK_END);
	fread(buffer,sizeof(char),30,ffd);
	safe_store(safe,buffer,60);
	mi->album=calloc(61,sizeof(char));
	strcpy(mi->album,safe);
	next+=30;

	fseek(ffd,next,SEEK_END);
	fread(buffer,sizeof(char),4,ffd);
	safe_store(safe,buffer,4);
	mi->year=calloc(9,sizeof(char));
	strcpy(mi->year,safe);
	next+=4;

	printf("v1: %s | %s | %s | %s| %s | %s || %d\n\n",mi->title,mi->track,mi->album,mi->artist,mi->length,mi->year,next);
}

struct musicInfo* plugin_meta(FILE *ffd){
	int MAX_VERSION=4;
	struct musicInfo *mi=calloc(1,sizeof(struct musicInfo));
	int next=0;
	unsigned char buffer[255];
	fseek(ffd,next,SEEK_SET);
	fread(buffer,sizeof(char),10,ffd);
	if(memcmp("ID3",buffer,3)==0 && buffer[4]<=MAX_VERSION){ // ID3v2
		ID3v2Parse(ffd,mi);
	}
	else{
		//fseek(ffd,0,SEEK_END);
		fseek(ffd,-128,SEEK_END);
		fread(buffer,sizeof(char),4,ffd);
		//fprintf(stderr,"%x %x %x\n",buffer[0],buffer[1],buffer[2]);
		if(memcmp("TAG",buffer,3)==0)ID3v1Parse(ffd,mi); // ID3v1
	}
	return mi;
}

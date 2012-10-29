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

#include "plugin.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/select.h>

#define S_DEF_PORT_STR "80"

struct streamHandles{
	FILE *rfd;
	FILE *wfd;
	int sfd;
	FILE *ffd;
	struct pluginitem *dec;
	int metaint;
	int bytecount;
	int print_meta;
	volatile char go;
}h;

#define S_BUFSIZE (1024)
#define S_DEF_TITLE "UnknownStream"

#define SI_NAME_SIZE (100)
#define SI_GENRE_SIZE (100)
#define SI_DESCRIPTION_SIZE (100)
#define SI_TYPE_SIZE (30)

struct streamInfo{
	char *name;
	char *genre;
	char *description;
	char *type;
	int bitrate;
};

static int open_decoder_plugin(struct playerHandles *ph){
	return 0;
}

static int open_pipe(){
	int pipefd[2];
	if(pipe(pipefd)){
		fprintf(stderr,"pipe error\n");
		return 1;
	}
	h.rfd=fdopen(pipefd[0],"rb");
	h.wfd=fdopen(pipefd[1],"wb");
	return 0;
}

static void close_pipe(){
	if(h.wfd){
		fclose(h.wfd);
		h.wfd=NULL;
	}
	if(h.rfd){
		fclose(h.rfd);
		h.rfd=NULL;
	}
}

void plugin_close(FILE *ffd){
	close_pipe();
	close(h.sfd);
}

static void parse_url(const char *path, char **orig_url, char **orig_port, char **orig_filename){
	int x;
	char *url=*orig_url;
	char *port=*orig_port;
	char *ptr;

	if(strncmp("http://",path,7)!=0)
		url=strdup(path);
	else
		url=strdup(path+7);
	if(!(port=malloc(10))){
		fprintf(stderr,"Malloc failed.");
		return;
	}
	*orig_port=port;
	*orig_url=url;

	for(x=0;url[x] && url[x]!=':';x++);
	if(url[x]==':'){
		url[x++]=0;
		while(url[x] && url[x]>='0' && url[x]<='9')*(port++)=url[x++];
		*port=0;
		if(*(url+x))
			*orig_filename=strdup(url+x);
		else{
			goto def_filename;
		}

	}
	else{
		strcpy(port,S_DEF_PORT_STR);
def_filename:
		if(!(*orig_filename=malloc(1))){
			fprintf(stderr,"Malloc failed.");
			return;
		}
		**orig_filename=0;
	}
}

static int stream_hello(char *filename){
	char hello[300];
	int hello_len;

	if(*filename!=0)
		sprintf(hello,"GET %s HTTP/1.0\r\nUser-Agent: HARP\r\nIcy-MetaData:%d\r\n\r\n",filename,h.print_meta);
	else
		sprintf(hello,"GET / HTTP/1.0\r\nUser-Agent: HARP\r\nIcy-MetaData:%d\r\n\r\n",h.print_meta);

	hello_len=strlen(hello);
	if(write(h.sfd,hello,hello_len)<hello_len){
		fprintf(stderr,"Short write.\n");
		return 1;
	}
	return 0;
}

FILE *plugin_open(const char *path, const char *mode){
	int sfd;
	int ret;
	int x;
	struct addrinfo hints,*rp,*result;
	char *url,*port,*filename;

	if(open_pipe())
		return NULL;

	parse_url(path,&url,&port,&filename);

	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_protocol=0;
	if((ret=getaddrinfo(url,port,&hints,&result))){
		fprintf(stderr,"error (%s) - getaddrinfo: %s\n",path,gai_strerror(ret));
		close_pipe();
		free(port);
		return NULL;
	}
	free(url);
	free(port);

	for(rp=result;rp;rp=rp->ai_next){
		if((sfd=socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol))==-1)
			continue;
		if(connect(sfd,rp->ai_addr,rp->ai_addrlen)!=-1){
			h.sfd=sfd;
			h.ffd=fdopen(sfd,mode);
			break;
		}
		close(sfd);
	}
	if(!rp){
		fprintf(stderr,"Cannot connect to: %s\n",path);
		close_pipe();
		return NULL;
	}
	freeaddrinfo(result);

	h.print_meta=1;
	if(stream_hello(filename)){
		plugin_close(NULL);
		return NULL;
	}
	free(filename);

	return h.rfd;
}

void plugin_seek(struct playerHandles *ph, int modtime){
	return;
}

int filetype_by_data(FILE *ffd){
	if(ffd==h.rfd)
		return 1;
	return 0;
}

static void print_stream_meta(struct streamInfo *si){
	if(si->name && *si->name)
		printf("Name:        %s\n",si->name);
	if(si->description && *si->description)
		printf("Description: %s\n",si->description);
	if(si->genre && *si->genre)
		printf("Genre:       %s\n",si->genre);
	if(si->bitrate)
		printf("Bitrate:     %d\n",si->bitrate);
	if(si->type && *si->type)
		printf("Type:        %s\n",si->type);
	printf("---------------------\n");
}

static int empty_parse(char *buf, int *len, void *data){
	return 0;
}

static char *get_line(char *buf, int len){
	int x;
	int nl=0;
	char *ptr=buf;

	if(len<1)return NULL;

	for(x=0;x<len;x++){
		if(!nl && *(ptr-1)==0){
			buf=ptr;
			if(len-x>=2 && strncmp(ptr,"\r\n",2)==0)
				break;
		}
		if(*ptr=='\r'){
			nl=1;
			*ptr=0;
		}
		else if(*ptr=='\n'){
			nl=1;
			*ptr=0;
			break;
		}
		ptr++;
	}

	return buf;
}

static char *split_icy(char *buf){
	if(!*buf)return NULL;

	while(*(++buf)){
		if(*buf==':'){
			*buf=0;
			return buf+1;
		}
	}

	return NULL;
}

static int parse_meta_si(char *buf, int *orig_len, void *data){
	static int ret=1; /* use 1 additional block */
	static void *last_data=NULL;
	int len=*orig_len;
	char *ptr=buf;
	char *val;
	int x;
	struct streamInfo *si=(struct streamInfo*)data;

	if(*buf<32 || *buf>126)return 0;

	if(last_data!=data)ret=1; /* Reset block counter if writing to new data */
	last_data=data;

	while((ptr=get_line(ptr,len-(ptr-buf)))){
		if(len-(ptr-buf) && !*ptr){
			*orig_len=len-((ptr)-buf);
			break;
		}
		if(strncmp(ptr,"\r\n",2)==0){
			*orig_len=len-((ptr+2)-buf);
			return 0;
		}
		if(!(val=split_icy(ptr))){
			continue;
		}
		if(strcmp(ptr,"icy-name")==0){
			strncpy(si->name,val,SI_NAME_SIZE);
		}
		if(strcmp(ptr,"icy-description")==0){
			strncpy(si->description,val,SI_DESCRIPTION_SIZE);
		}
		if(strcmp(ptr,"icy-genre")==0){
			strncpy(si->genre,val,SI_GENRE_SIZE);
		}
		if(strcmp(ptr,"icy-br")==0){
			si->bitrate=(int)strtol(val,NULL,10);
		}
		if(strcmp(ptr,"icy-metaint")==0){
			h.metaint=(int)strtol(val,NULL,10);
		}
		if(strcasecmp(ptr,"Content-Type")==0){
			while(*val){
				if(*val==' ')val++;
				else break;
			}
			while(*val && *(val++)!='/');
			strncpy(si->type,val,SI_TYPE_SIZE);
		}
	}
	len-=(ptr-buf);
	return ret--;
}

static int parse_meta_mi(char *buf, int *orig_len, void *data){
	static int ret=1; /* use 1 additional block */
	static void *last_data=NULL;
	int len=*orig_len;
	char *ptr=buf;
	char *val;
	int x;
	struct musicInfo *mi=(struct musicInfo*)data;

	if(*buf<32 || *buf>126)return 0;

	if(last_data!=data)ret=1; /* Reset block counter if writing to new data */
	last_data=data;

	while((ptr=get_line(ptr,len-(ptr-buf)))){
		if(len-(ptr-buf) && !*ptr){
			break;
		}
		if(strncmp(ptr,"\r\n",2)==0){
			len=len-((ptr+2)-buf);
			return 0;
		}
		if(!(val=split_icy(ptr))){
			continue;
		}
		if(strcmp(ptr,"icy-name")==0){
			strncpy(mi->title,val,MI_TITLE_SIZE);
		}
	}
	len-=(ptr-buf);
	return ret--;
}

static char *print_streamtitle(char *buf, int buf_size, int *meta_len){
	if(*meta_len>buf_size)
		buf_size-=*meta_len;
	else
		buf_size=0;
	for(;*meta_len>buf_size;(*meta_len)--,buf++)
		if(*buf)
			printf("%c",*buf<32?'?':*buf);
	if(!*meta_len)
		putchar('\n');
		else putchar('$');
	return buf;
}

static char *handle_streamtitle(char *buf, int *len, int *metasize){
	int temp=*metasize;
	buf=print_streamtitle(buf,*len,metasize);
	*len-=temp-*metasize;
	return buf;
}

static int write_pipe_parse_meta(char *buf, int *len, void *data){
	int ret,temp;
	static int metasize=0;
	static int save;
	char *ptr=buf;

	if(!h.wfd){
		fprintf(stderr,"No wfd\n");
		return 0;
	}
	if(h.bytecount+*len>h.metaint){
		volatile int *up=&(((struct playerHandles *)data)->pflag->update);
		
		temp=h.metaint-h.bytecount;
		if((ret=fwrite(ptr,1,temp,h.wfd))<temp)
			fprintf(stderr,"Short write to pipe\n");
		ptr+=ret+1;
		*len-=ret+1;
		h.bytecount+=ret; // Should = metaint now

		if(!metasize){
			save=*up;
			*up=0;
			metasize=(int)((char)*(ptr-1))*16;
			if(!h.print_meta || metasize<0) // Cancel all further printing
				metasize=h.print_meta=0;
		}

		if(metasize)
			ptr=handle_streamtitle(ptr,len,&metasize);

		if(metasize<=0){
			*up=save;
			metasize=h.bytecount=0;
		}
		if(*len<1)return 1;
	}

	if((ret=fwrite(ptr,1,*len,h.wfd))<*len){
		fprintf(stderr,"Short write to pipe\n");
	}
	h.bytecount+=ret;
	return 1;
}

static int write_pipe(char *buf, int *len, void *data){
	int ret;

	if(!h.wfd){
		fprintf(stderr,"No wfd\n");
		return 0;
	}

	if((ret=fwrite(buf,1,*len,h.wfd))<*len){
		fprintf(stderr,"Short write to pipe\n");
	}
	return 1;
}

static void streamIO(int parse(char*,int*,void*),void *data){
	int len,ret,x;
	char buf[S_BUFSIZE+1];
	buf[S_BUFSIZE]=0;

	while(1){
		if(!h.go)
			break;

		len=ret=read(h.sfd,buf,S_BUFSIZE);
		if(!parse(buf,&len,data))
			break;
	}
	if(h.metaint){
		h.bytecount=len;
	}
}

void plugin_meta(FILE *ffd, struct musicInfo *mi){
	streamIO(parse_meta_mi,mi);

	if(!(*mi->title))
		strncpy(mi->title,S_DEF_TITLE,MI_TITLE_SIZE);
	mi->length=-1;
}

static char *nextType(char *type){
	int x;
	for(x=0;type[x];x++){
		if(type[x]==';'){
			return type+x+1;
		}
	}
	return NULL;
}

static struct pluginitem *selectPlugin(struct pluginitem *list, char *type){
	struct pluginitem *ret=list;
	char buf[S_BUFSIZE],*ptr;
	int len,tlen,x;
	if(type==NULL || *type==0)return NULL;
	len=strlen(type);
	
	while(ret){
		ptr=ret->contenttype;
		while(ptr){
			if(strncmp(ptr,type,len)==0)
				return ret;
			ptr=nextType(ptr);
		}
		ret=ret->next;
	}
	return NULL;
}

void* sio_thread(void *data){
	if(h.print_meta && h.metaint)
		streamIO(write_pipe_parse_meta,data);
	else
		streamIO(write_pipe,data);

	return (void*)0;
}

int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	int ret,temp=-1;
	struct pluginitem *plugin;
	struct streamInfo si;
	pthread_t threads;
	char buf[S_BUFSIZE+1];
	fd_set fdset;
	struct timeval timeout;
	int rfd;

	if(!(si.name=malloc((SI_NAME_SIZE+1)*sizeof(char))) ||
		!(si.description=malloc((SI_DESCRIPTION_SIZE+1)*sizeof(char))) ||
		!(si.genre=malloc((SI_GENRE_SIZE+1)*sizeof(char))) ||
		!(si.type=malloc((SI_TYPE_SIZE+1)*sizeof(char)))){
		fprintf(stderr,"Can't malloc for si\n");
		return DEC_RET_ERROR;
	}
	memset(si.name,0,SI_NAME_SIZE);
	memset(si.genre,0,SI_GENRE_SIZE);
	memset(si.description,0,SI_DESCRIPTION_SIZE);
	memset(si.type,0,SI_TYPE_SIZE);
	si.bitrate=0;
	
	h.metaint=h.bytecount=0;
	h.go=1;
	streamIO(parse_meta_si,&si);
	print_stream_meta(&si);
	if(!(plugin=selectPlugin(ph->plugin_head,si.type))){
		fprintf(stderr,"No plugin matches content-type. Trying first plugin.\n");
		plugin=ph->plugin_head;
	}

	ph->ffd=h.rfd;
	pthread_create(&threads,NULL,(void *)&sio_thread,(void *)ph);
	ret=plugin->modplay(ph,key,&temp);

	h.go=0;

#ifdef __APPLE__
	rfd=fileno(h.rfd);
	do{
		timeout.tv_sec=0;
		timeout.tv_usec=100000;
		FD_ZERO(&fdset);
		FD_SET(rfd,&fdset);
		if(select(1,&fdset,NULL,NULL,&timeout)<1)
			break;
	}while((temp=fread(buf,1,S_BUFSIZE,h.rfd))==S_BUFSIZE);

	// OSX seems to block when closing the write end unless the read end is first closed
	close(rfd);
	h.rfd=NULL;
#else
	if(pthread_cancel(threads)!=0)fprintf(stderr,"Failed cancel.\n");
	if(pthread_join(threads,NULL)!=0)fprintf(stderr,"Failed join\n");
#endif

	//usleep(100000);

	return ret;
}

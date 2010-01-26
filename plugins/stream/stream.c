/*
 *  Copyright (C) 2009-2010  Christian Heckendorf <heckendorfc@gmail.com>
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

#define S_DEF_PORT_STR "80"
struct streamHandles{
	FILE *rfd;
	FILE *wfd;
	int sfd;
	FILE *ffd;
	struct pluginitem *dec;
}h;

static int open_decoder_plugin(struct playerHandles *ph){
	return 0;
}

int open_pipe(){
	int pipefd[2];
	if(pipe(pipefd)){
		fprintf(stderr,"pipe error\n");
		return 1;
	}
	h.rfd=fdopen(pipefd[0],"rb");
	h.wfd=fdopen(pipefd[1],"wb");
	return 0;
}

void close_pipe(){
	if(h.rfd){
		fclose(h.rfd);
		h.rfd=NULL;
	}
	if(h.wfd){
		fclose(h.wfd);
		h.wfd=NULL;
	}
}

FILE *plugin_open(const char *path, const char *mode){
	int sfd;
	int ret;
	int x;
	struct addrinfo hints,*rp,*result;
	char *url,*port;
	if(strncmp("http://",path,7)!=0)
		url=strdup(path);
	else
		url=strdup(path+7);

	if(open_pipe())
		return NULL;

	for(x=0;url[x];x++);
	for(--x;x;x--){
		if(url[x]==':'){
			url[x++]=0;
			break;
		}
	}
	if(x)
		port=strdup(url+x);
	else{
		if(!(port=malloc(3))){
			fprintf(stderr,"Malloc failed.");
			return NULL;
		}
		strcpy(port,S_DEF_PORT_STR);
	}

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

	//fprintf(stderr,"Opened conn.\n");
	return h.rfd;
}

void plugin_close(FILE *ffd){
	close_pipe();
	close(h.sfd);
	//fprintf(stderr,"Closed conn.\n");
}

void plugin_seek(struct playerHandles *ph, int modtime){
	return;
}

int filetype_by_data(FILE *ffd){
	if(ffd==h.rfd)
		return 1;
	return 0;
}

#define S_BUFSIZE (512)
#define S_DEF_TITLE "UnknownStream"

static int empty_parse(char *buf, int len, void *data){
	return 0;
}

static char *get_line(char *buf, int len){
	int x;
	int nl=0;
	char *ptr=buf;

	if(len<1)return NULL;

	for(x=0;x<len;x++){
		if(!nl && *(ptr-1)==0)buf=ptr;
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

static int parse_meta(char *buf, int len, void *data){
	static int ret=1; /* use 1 additional block */
	static void *last_data=NULL;
	char *ptr=buf;
	char *val;
	struct musicInfo *mi=(struct musicInfo*)data;

	if(last_data!=data)ret=1; /* Reset block counter if writing to new data */
	last_data=data;

	while((ptr=get_line(ptr,len-(ptr-buf))) && *ptr){
		//printf("line: %s\n",ptr);
		if(!(val=split_icy(ptr)))
			continue;
		if(strcmp(ptr,"icy-name")==0){
			strncpy(mi->title,val,MI_TITLE_SIZE);
		}
	}

	return ret--;
}

static void streamIO(int parse(char*,int,void*),void *data){
	const char hello[]="GET / HTTP/1.0\r\nIcy-MetaData:0\r\n\r\n";
	const int hello_len=strlen(hello);
	int ret;
	char buf[S_BUFSIZE+1];
	buf[S_BUFSIZE]=0;

	if(write(h.sfd,hello,hello_len)<hello_len)
		fprintf(stderr,"Short write.\n");

	while(1){
		ret=read(h.sfd,buf,S_BUFSIZE);
		//printf("Recieved %d bytes\n%s\n",ret,buf);
		//printf("Recieved %d bytes\n",ret);
		if(!parse(buf,ret,data))
			break;
	}
}

void plugin_meta(FILE *ffd, struct musicInfo *mi){
	streamIO(parse_meta,mi);
	if(!(*mi->title))
		strncpy(mi->title,S_DEF_TITLE,MI_TITLE_SIZE);
	mi->length=-1;
}

int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	int ret,temp=-1;
	//char buf[S_BUFSIZE+1];
	//buf[S_BUFSIZE]=0;
	//const char hello[]="GET / HTTP/1.0\r\nHost: streamb.wgbh.org:8000\r\nUser-Agent: HARP\r\nIcy-MetaData: 1\r\n";

	streamIO(empty_parse,NULL);
	ph->ffd=h.ffd;
	ret=ph->plugin_head->modplay(ph,key,&temp);
	return ret;
}

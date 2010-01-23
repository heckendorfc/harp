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
	char *url;
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

	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_protocol=0;
	if((ret=getaddrinfo(url,url+x,&hints,&result))){
		fprintf(stderr,"error - getaddrinfo: %s\n",gai_strerror(ret));
		close_pipe();
		return NULL;
	}

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
		free(url);
		close_pipe();
		return NULL;
	}
	freeaddrinfo(result);
	free(url);

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

void plugin_meta(FILE *ffd, struct musicInfo *mi){
	strcpy(mi->title,"streamtest");
}

#define S_BUFSIZE (512)

char *parse_meta(char *buf){
	char *ptr=buf;

	return ptr;
}

static void streamIO(struct playerHandles *ph){
	const char hello[]="GET / HTTP/1.0\r\nIcy-MetaData:0\r\n\r\n";
	const int hello_len=strlen(hello);
	int ret;
	char buf[S_BUFSIZE];
	if(write(h.sfd,hello,hello_len)<hello_len)
		fprintf(stderr,"Short write.\n");
	while(1){
		ret=read(h.sfd,buf,S_BUFSIZE);
		//printf("Recieved %d bytes\n%s\n",ret,buf);
		parse_meta(buf);
		break;
	}
}

int plugin_run(struct playerHandles *ph, char *key, int *totaltime){
	char buf[S_BUFSIZE];
	int ret,temp;
	//const char hello[]="GET / HTTP/1.0\r\nHost: streamb.wgbh.org:8000\r\nUser-Agent: MPlayer/SVN-r29796-4.3.4\r\nIcy-MetaData: 1\r\n";
/*
	pthread_t threads;
	pthread_mutex_init(&actkey,NULL);
	pthread_create(&threads,NULL,(void *)&streamIO,(void*)ph);
*/
	streamIO(ph);

	ph->ffd=h.ffd;
	ret=ph->plugin_head->modplay(ph,key,&temp);
	return ret;
}

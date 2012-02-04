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

#include "message.h"
#include "defs.h"

void debug(const int level, const char *msg){
#ifndef _HARP_PLUGIN
	if(arglist[AVERBOSE].active>=level)
		fprintf(stderr,"%s\n",msg);
	if(debugconf.log==1 && debugconf.level>=level && debugconf.msgfd){
		fprintf(debugconf.msgfd,"%s\n",msg);
		fflush(debugconf.msgfd);
	}
#else
#ifdef HARP_PLUGIN_DEBUG
	if(HARP_PLUGIN_DEBUG>=level)
		fprintf(stderr,"%s\n",msg);
#endif
#endif
}

#ifndef _HARP_PLUGIN
void printSongPubInfo(char **row){
	printf( "\n=====================\n"
			"Title:     %s\n"
			"Artist:    %s\n"
			"Album:     %s\n"
			"Location:  %s\n"
			"File Type: %s\n"
			"---------------------\n",
			row[0],row[3],row[2],row[1],row[4]);
	if(debugconf.log==1 && debugconf.playfd){
		fprintf(debugconf.playfd,"%s\t%s\t%s\t%s\t%s\n",row[0],row[3],row[2],row[1],row[4]);
		fflush(debugconf.playfd);
	}
}
#endif

void addStatusTail(char *msg, struct outputdetail *detail){
	int len=0;
	pthread_mutex_lock(&outstatus);
		len=snprintf(detail->tail,OUTPUT_TAIL_SIZE,"    %s",msg);
		for(;len<OUTPUT_TAIL_SIZE-2;len++)detail->tail[len]=' ';
		detail->tail[OUTPUT_TAIL_SIZE-1]=0;
		detail->tail[OUTPUT_TAIL_SIZE-2]='\r';
	pthread_mutex_unlock(&outstatus);
}


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

#include "harp.h"

int main(int argc, char *argv[]){
	(void) signal(SIGINT,int_leave);
	if(!dbInit()){
		fprintf(stderr,"db init error\n");
		return 1;
	}
	sleep(10);
	doArgs(argc,argv);
	cleanExit();
	return 0;
}

void int_leave(int sig){
	cleanExit();
	exit(sig);
}

void cleanExit(){
	sqlite3_close(conn);
	debug("done -- database connection closed");
}

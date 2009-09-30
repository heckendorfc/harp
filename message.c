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

void debug(const char *argv){
	if(arglist[AVERBOSE].active>0)
		fprintf(stderr,"%s\n",argv);
}

void debug3(const char *argv){
	if(arglist[AVERBOSE].active>2)
		fprintf(stderr,"%s\n",argv);
}

void printSongPubInfo(char **row){
	printf("Title: %s\n",row[0]);
	printf("Artist: %s\n",row[3]);
	printf("Album: %s\n",row[2]);
	printf("Location: %s\n",row[1]);
	printf("File Type: %s\n",row[4]);
}

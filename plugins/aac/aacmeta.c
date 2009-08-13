/*
 *  Copyright (C) 2009  Christian Heckendorf <vaseros@gmail.com>
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


struct musicInfo* plugin_meta(FILE *ffd){
	struct musicInfo *mi=calloc(1,sizeof(struct musicInfo));
	char single,buf[4],song[61],artist[61],album[61];
	//int artistid,albumid;
	int x,y;
	fseek(ffd,0,SEEK_SET);
	for(x=0;x<61;x++)
		song[x]=artist[x]=album[x]=0;

	for(x=0;x<50000;x++){
		//fseek(ffd,x*sizeof(buf),SEEK_SET);

		fread(&single,sizeof(single),1,ffd);
		buf[x%4]=single;
		
		if(buf[(x-3)%4]==0xa9 && buf[(x-2)%4]=='n' && buf[(x-1)%4]=='a' && buf[(x)%4]=='m'){
			fseek(ffd,(16*sizeof(buf[0])),SEEK_CUR);
			for(y=0;y<30 && single!=0 ;y++){
				fread(&single,sizeof(single),1,ffd);
				if(single==0 || (single>31 && single<127))
					song[y]=single;
				else
					song[y]=' ';
			}
			song[y]=0;
			mi->title=calloc(61,sizeof(char));
			strcpy(mi->title,song);
		}

		if(buf[(x-3)%4]==0xa9 && buf[(x-2)%4]=='A' && buf[(x-1)%4]=='R' && buf[(x)%4]=='T'){
			fseek(ffd,(16*sizeof(buf[0])),SEEK_CUR);
			for(y=0;y<30 && single!=0 ;y++){
				fread(&single,sizeof(single),1,ffd);
				if(single==0 || (single>31 && single<127))
					artist[y]=single;
				else
					artist[y]=' ';
			}
			artist[y]=0;
			mi->artist=calloc(61,sizeof(char));
			strcpy(mi->artist,artist);
		}
		if(buf[(x-3)%4]==0xa9 && buf[(x-2)%4]=='a' && buf[(x-1)%4]=='l' && buf[(x)%4]=='b'){
			fseek(ffd,(16*sizeof(buf[0])),SEEK_CUR);
			for(y=0;y<30 && single!=0;y++){
				fread(&single,sizeof(single),1,ffd);
				if(single==0 || (single>31 && single<127))
					album[y]=single;
				else
					album[y]=' ';
			}
			album[y]=0;
			mi->album=calloc(61,sizeof(char));
			strcpy(mi->album,album);
		}
	}
	return mi;
}

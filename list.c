/*
 *  Copyright (C) 2009  Christian Heckendorf <heckendorfc@gmail.com>
 *
 *  This program is free software: you can redistribute it AND/or modify
 *  it under the terms of the GNU General Public License AS published by
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


int list(int *ids, int length){
	char query[401];
	int x,y,*exception;
	if(!(exception=malloc(sizeof(int)*10))){
		debug(2,"Malloc failed (exception).");
		return 1;
	}
	for(x=1;x<10;x++)exception[x]=listconf.exception;exception[0]=1;
	switch(arglist[ATYPE].subarg[0]){
		case 's':
			exception[1]=exception[2]=exception[3]=exception[4]=1;
			for(x=0;x<length;x++){
				sprintf(query,"SELECT SongID,SongTitle,Location,AlbumTitle,ArtistName FROM SongPubInfo WHERE SongID=%d ORDER BY ArtistName,AlbumTitle",ids[x]);
				doTitleQuery(query,exception,listconf.maxwidth);
				printf("\n");
			}
			break;

		case 'p':
			exception[0]=exception[1]=1;
			for(x=0;x<length;x++){
				sprintf(query,"SELECT PlaylistID,Title FROM Playlist WHERE PlaylistID=%d",ids[x]);
				doTitleQuery(query,exception,listconf.maxwidth);
				printf("\nContents:\n");

				sprintf(query,"SELECT `Order`, SongID,SongTitle,AlbumTitle,ArtistName FROM PlaylistSong NATURAL JOIN SongPubInfo WHERE PlaylistID=%d ORDER BY `Order`",ids[x]);
				printf("------\nTotal:%d\n",doTitleQuery(query,exception,listconf.maxwidth));
			}
			break;

		case 'r':
			for(x=0;x<length;x++){
				sprintf(query,"SELECT Artist.ArtistID, Artist.Name FROM Artist WHERE ArtistID=%d",ids[x]);
				doTitleQuery(query,exception,listconf.maxwidth);
				printf("\nContents:\n");
				
				sprintf(query,"SELECT Song.SongID, Song.Title, Album.Title AS Album, Artist.Name AS Artist FROM Artist NATURAL JOIN AlbumArtist NATURAL JOIN Album INNER JOIN Song USING(AlbumID) WHERE Artist.ArtistID=%d ORDER BY Artist.Name, Album.Title",ids[x]);
				printf("------\nTotal:%d\n",doTitleQuery(query,exception,listconf.maxwidth));
			}
			break;

		case 'a':
			for(x=0;x<length;x++){
				sprintf(query,"SELECT Album.AlbumID,Album.Title FROM Album WHERE AlbumID=%d",ids[x]);
				doTitleQuery(query,exception,listconf.maxwidth);
				printf("\nContents:\n");

				sprintf(query,"SELECT Song.SongID, Song.Title, Album.Title AS Album, Artist.Name AS Artist FROM Artist NATURAL JOIN AlbumArtist NATURAL JOIN Album INNER JOIN Song USING(AlbumID) WHERE Album.AlbumID=%d ORDER BY Artist.Name, Album.Title",ids[x]);
				printf("------\nTotal:%d\n",doTitleQuery(query,exception,listconf.maxwidth));
			}
			break;

		case 'g':
			for(x=0;x<length;x++){
				printGenreTree(ids[x],(void *)tierChildPrint);
			}
			break;
	}
	free(exception);
	return 0;
}

int listall(){
	char query[401];
	int x,*headpath,*exception;
	if(!(exception=malloc(sizeof(int)*10))){
		debug(2,"Malloc failed (exception).");
		return 1;
	}
	for(x=1;x<10;x++)exception[x]=listconf.exception;exception[0]=1;
	switch(arglist[ATYPE].subarg[0]){
		case 's':
			sprintf(query,"SELECT Song.SongID, Song.Title, Song.Location, Album.Title AS Album, Artist.Name AS Artist FROM Song,Album,Artist,AlbumArtist WHERE Song.AlbumID=Album.AlbumID AND Album.AlbumID=AlbumArtist.AlbumID AND AlbumArtist.ArtistID=Artist.ArtistID ORDER BY Artist.Name, Album.Title");
			exception[1]=exception[2]=exception[3]=exception[4]=1;
			break;

		case 'p':
			sprintf(query,"SELECT PlaylistID,Title FROM Playlist");
			break;

		case 'r':
			sprintf(query,"SELECT Artist.ArtistID, Artist.Name FROM Artist");
			break;

		case 'a':
			sprintf(query,"SELECT Album.AlbumID,Album.Title FROM Album");
			break;

		case 'g':
			printGenreTree(0,(void *)tierCatPrint);
			return 0;
		default:return 1;
	}
	doTitleQuery(query,exception,listconf.maxwidth);
	free(exception);
	return 0;
}

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


int list(int id){
	char query[401];
	int x,*exception=alloca(sizeof(int)*10);
	for(x=1;x<10;x++)exception[x]=listconf.exception;exception[0]=1;
	struct dbitem dbi;
	dbiInit(&dbi);
	switch(arglist[ATYPE].subarg[0]){
		case 's':
			sprintf(query,"select Song.SongID, Song.Title, Song.Location, Album.Title as Album, Artist.Name as Artist from Song,Album,Artist,AlbumArtist where Song.AlbumID=Album.AlbumID and Album.AlbumID=AlbumArtist.AlbumID and AlbumArtist.ArtistID=Artist.ArtistID and SongID=%d order by Artist.Name, Album.Title",id);
			exception[1]=exception[2]=exception[3]=exception[4]=1;
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			break;

		case 'p':
			sprintf(query,"select PlaylistID,Title from Playlist where PlaylistID=%d",id);
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			printf("\nContents:\n");

			sprintf(query,"select `Order`, Song.SongID, Song.Title, Album.Title as Album, Artist.Name as Artist from PlaylistSong,Song,Album,Artist,AlbumArtist where PlaylistSong.SongID=Song.SongID and Song.AlbumID=Album.AlbumID and Album.AlbumID=AlbumArtist.AlbumID and AlbumArtist.ArtistID=Artist.ArtistID and PlaylistID=%d order by `Order`",id);
			exception[0]=exception[1]=1;
			printf("------\nTotal:%d\n",doTitleQuery(query,&dbi,exception,listconf.maxwidth));
			break;

		case 'r':
			sprintf(query,"select Artist.ArtistID, Artist.Name from Artist where ArtistID=%d",id);
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			printf("\nContents:\n");

			sprintf(query,"select Song.SongID, Song.Title, Album.Title as Album, Artist.Name as Artist from Song,Album,Artist,AlbumArtist where Song.AlbumID=Album.AlbumID and Album.AlbumID=AlbumArtist.AlbumID and AlbumArtist.ArtistID=Artist.ArtistID and Artist.ArtistID=%d order by Artist.Name, Album.Title",id);
			printf("------\nTotal:%d\n",doTitleQuery(query,&dbi,exception,listconf.maxwidth));
			break;

		case 'a':
			sprintf(query,"select Album.AlbumID,Album.Title from Album where AlbumID=%d",id);
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			printf("\nContents:\n");

			sprintf(query,"select Song.SongID, Song.Title, Album.Title as Album, Artist.Name as Artist from Song,Album,Artist,AlbumArtist where Song.AlbumID=Album.AlbumID and Album.AlbumID=AlbumArtist.AlbumID and AlbumArtist.ArtistID=Artist.ArtistID and Album.AlbumID=%d order by Artist.Name, Album.Title",id);
			printf("------\nTotal:%d\n",doTitleQuery(query,&dbi,exception,listconf.maxwidth));
			break;
	}
	return 0;
}

int listall(){
	char query[401];
	int x,*exception=alloca(sizeof(int)*10);
	for(x=1;x<10;x++)exception[x]=listconf.exception;exception[0]=1;
	struct dbitem dbi;
	dbiInit(&dbi);
	switch(arglist[ATYPE].subarg[0]){
		case 's':
			sprintf(query,"select Song.SongID, Song.Title, Song.Location, Album.Title as Album, Artist.Name as Artist from Song,Album,Artist,AlbumArtist where Song.AlbumID=Album.AlbumID and Album.AlbumID=AlbumArtist.AlbumID and AlbumArtist.ArtistID=Artist.ArtistID order by Artist.Name, Album.Title");
			exception[1]=exception[2]=exception[3]=exception[4]=1;
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			break;

		case 'p':
			sprintf(query,"select PlaylistID,Title from Playlist");
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			break;

		case 'r':
			sprintf(query,"select Artist.ArtistID, Artist.Name from Artist");
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			break;

		case 'a':
			sprintf(query,"select Album.AlbumID,Album.Title from Album");
			doTitleQuery(query,&dbi,exception,listconf.maxwidth);
			break;
	}
	return 0;
}


CREATE TABLE Category(
CategoryID integer primary key,
Name varchar(50) not null,
ParentID integer not null default 0);

CREATE TABLE SongCategory(
SongCatID integer primary key,
CategoryID integer not null,
SongID integer not null);

CREATE TABLE FileType(
TypeID integer primary key,
Name varchar(10) not null UNIQUE);

CREATE TABLE PluginType(
PluginTypeID integer primary key,
TypeID integer not null,
PluginID integer not null,
Active integer not null default 1);

CREATE TABLE Plugin(
PluginID integer primary key,
Library varchar(200) not null UNIQUE);

CREATE TABLE FileExtension(
ExtensionID integer primary key,
TypeID integer not null,
Extension varchar(10) not null UNIQUE);

CREATE TABLE Song(
SongID integer primary key,
Title varchar(200) not null,
Location varchar(250) not null,
TypeID integer(3) not null default 1,
AlbumID integer not null,
Track integer not null default 0,
Rating integer(3) not null default 3,
PlayCount integer not null default 0,
SkipCount integer not null default 0,
LastPlay integer(11) not null default 0,
Length integer not null default -1,
Active integer(1) not null default 1);

CREATE TABLE PlaylistSong(
PlaylistSongID integer primary key,
SongID integer not null,
PlaylistID integer not null,
`Order` integer NOT NULL);

CREATE TABLE Playlist(
PlaylistID integer primary key,
Title varchar(50) not null);

CREATE TABLE Album(
AlbumID integer primary key,
Title varchar(100) not null,
`Date` integer(11));

CREATE TABLE AlbumArtist(
AlbumArtistID integer primary key,
ArtistID integer not null,
AlbumID integer not null);

CREATE TABLE Artist(
ArtistID integer primary key,
Name varchar(100) not null);

CREATE VIEW FilePlugin AS
SELECT FileType.TypeID AS TypeID, Name, Library
FROM FileType NATURAL JOIN PluginType NATURAL JOIN Plugin
ORDER BY Active DESC;

CREATE VIEW SongPubInfo AS
SELECT Song.Title AS SongTitle, Song.Location AS Location,
Album.Title AS AlbumTitle, Artist.Name AS ArtistName,
FilePlugin.Name AS FilePluginName, Song.Length AS Length,
FilePlugin.Library AS Library, Song.Rating AS Rating,
Song.SongID AS SongID, Song.Track AS SongTrack
FROM FilePlugin
INNER JOIN Song ON FilePlugin.TypeID=Song.TypeID
INNER JOIN Album ON Song.AlbumID=Album.AlbumID
NATURAL JOIN AlbumArtist
INNER JOIN Artist ON AlbumArtist.ArtistID=Artist.ArtistID;

INSERT INTO Playlist(PlaylistID,Title) VALUES (1,'Library');
INSERT INTO Artist(ArtistID,Name) VALUES (1,'Unknown');
INSERT INTO Album(AlbumID,Title) VALUES (1,'Unknown');
INSERT INTO AlbumArtist(ArtistID,AlbumID) VALUES (1,1);
INSERT INTO Category(CategoryID,Name) VALUES(1,"Unknown");

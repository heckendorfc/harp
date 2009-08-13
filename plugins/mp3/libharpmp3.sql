INSERT OR IGNORE INTO FileType(Name) VALUES('MP3');
INSERT OR IGNORE INTO Plugin(Library) VALUES('libharpmp3.so');
INSERT INTO PluginType(TypeID,PluginID)
SELECT TypeID,(SELECT PluginID FROM Plugin WHERE Library='libharpmp3.so')
FROM FileType WHERE Name='MP3';

INSERT OR IGNORE INTO FileExtension(TypeID,Extension)
SELECT TypeID,'mp3' FROM FileType WHERE Name='MP3' LIMIT 1;

DELETE FROM PluginType WHERE PluginTypeID IN
(SELECT DISTINCT a.PluginTypeID FROM PluginType a, PluginType b
WHERE a.TypeID=b.TypeID AND a.PluginID=b.PluginID AND
a.PluginTypeID>b.PluginTypeID);

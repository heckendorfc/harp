INSERT OR IGNORE INTO FileType(Name) VALUES('VORBIS');
INSERT OR IGNORE INTO Plugin(Library) VALUES('libharpvorbis.so');
INSERT INTO PluginType(TypeID,PluginID)
SELECT TypeID,(SELECT PluginID FROM Plugin WHERE Library='libharpvorbis.so')
FROM FileType WHERE Name='VORBIS';

INSERT OR IGNORE INTO FileExtension(TypeID,Extension)
SELECT TypeID,"ogg" FROM FileType WHERE Name='VORBIS' LIMIT 1;
INSERT OR IGNORE INTO FileExtension(TypeID,Extension)
SELECT TypeID,"oga" FROM FileType WHERE Name='VORBIS' LIMIT 1;


DELETE FROM PluginType WHERE PluginTypeID IN
(SELECT DISTINCT a.PluginTypeID FROM PluginType a, PluginType b
WHERE a.TypeID=b.TypeID AND a.PluginID=b.PluginID AND
a.PluginTypeID>b.PluginTypeID);

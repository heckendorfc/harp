INSERT OR IGNORE INTO FileType(Name,ContentType) VALUES('FLAC','x-flac');
INSERT OR IGNORE INTO Plugin(Library) VALUES('libharpflac.so');
INSERT INTO PluginType(TypeID,PluginID)
SELECT TypeID,(SELECT PluginID FROM Plugin WHERE Library='libharpflac.so')
FROM FileType WHERE Name='FLAC';

INSERT OR IGNORE INTO FileExtension(TypeID,Extension)
SELECT TypeID,"flac" FROM FileType WHERE Name='FLAC' LIMIT 1;


DELETE FROM PluginType WHERE PluginTypeID IN
(SELECT DISTINCT a.PluginTypeID FROM PluginType a, PluginType b
WHERE a.TypeID=b.TypeID AND a.PluginID=b.PluginID AND
a.PluginTypeID>b.PluginTypeID);

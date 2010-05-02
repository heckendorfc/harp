INSERT OR IGNORE INTO FileType(Name,ContentType) VALUES('STREAM','null');
INSERT OR IGNORE INTO Plugin(Library) VALUES('libharpstream.so');
INSERT INTO PluginType(TypeID,PluginID)
SELECT TypeID,(SELECT PluginID FROM Plugin WHERE Library='libharpstream.so')
FROM FileType WHERE Name='STREAM';

INSERT OR IGNORE INTO FileExtension(TypeID,Extension)
SELECT TypeID,'///' FROM FileType WHERE Name='STREAM' LIMIT 1;

DELETE FROM PluginType WHERE PluginTypeID IN
(SELECT DISTINCT a.PluginTypeID FROM PluginType a, PluginType b
WHERE a.TypeID=b.TypeID AND a.PluginID=b.PluginID AND
a.PluginTypeID>b.PluginTypeID);

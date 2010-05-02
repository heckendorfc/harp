INSERT OR IGNORE INTO FileType(Name,ContentType) VALUES('AAC','aac;mp4');
INSERT OR IGNORE INTO Plugin(Library) VALUES('libharpaac.so');
INSERT INTO PluginType(TypeID,PluginID)
SELECT TypeID,(SELECT PluginID FROM Plugin WHERE Library='libharpaac.so')
FROM FileType WHERE Name='AAC';

INSERT OR IGNORE INTO FileExtension(TypeID,Extension)
SELECT TypeID,"m4a" FROM FileType WHERE Name='AAC' LIMIT 1;
INSERT OR IGNORE INTO FileExtension(TypeID,Extension)
SELECT TypeID,"m4b" FROM FileType WHERE Name='AAC' LIMIT 1;
INSERT OR IGNORE INTO FileExtension(TypeID,Extension)
SELECT TypeID,"aac" FROM FileType WHERE Name='AAC' LIMIT 1;
INSERT OR IGNORE INTO FileExtension(TypeID,Extension)
SELECT TypeID,"mp4" FROM FileType WHERE Name='AAC' LIMIT 1;


DELETE FROM PluginType WHERE PluginTypeID IN
(SELECT DISTINCT a.PluginTypeID FROM PluginType a, PluginType b
WHERE a.TypeID=b.TypeID AND a.PluginID=b.PluginID AND
a.PluginTypeID>b.PluginTypeID);

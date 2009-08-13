lib_LTLIBRARIES = libharpvorbis.la
libharpvorbis_la_CFLAGS = $(CFLAGS)
libharpvorbis_la_LDFLAGS = -module -avoid-version -lvorbisfile $(SOUND_OUT)
libharpvorbis_la_SOURCES = vorbis.c
INCLUDES = -I../.. -I..
pkgdata_DATA = libharpvorbis.sql

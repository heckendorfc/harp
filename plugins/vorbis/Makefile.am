lib_LTLIBRARIES = libharpvorbis.la
libharpvorbis_la_CFLAGS = $(CFLAGS)
libharpvorbis_la_LDFLAGS = -module -avoid-version -lvorbisfile $(SOUND_OUT)
libharpvorbis_la_SOURCES = vorbis.c
libharpvorbis_la_LIBADD = ../libharpplugin.la
INCLUDES = -I../.. -I..
pkgdata_DATA = libharpvorbis.sql

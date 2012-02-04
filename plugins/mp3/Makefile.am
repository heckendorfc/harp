lib_LTLIBRARIES = libharpmp3.la
libharpmp3_la_LDFLAGS = -module -avoid-version -lmpg123 $(SOUND_OUT)
libharpmp3_la_CFLAGS = $(CFLAGS)
libharpmp3_la_SOURCES = mp3.c
libharpmp3_la_LIBADD = ../libharpplugin.la
INCLUDES = -I../.. -I..
pkgdata_DATA = libharpmp3.sql

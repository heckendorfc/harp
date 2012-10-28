lib_LTLIBRARIES = libharpstream.la
pkgdata_DATA = libharpstream.sql
INCLUDES = -I../.. -I..
libharpstream_la_LDFLAGS = -module -avoid-version $(SOUND_OUT)
libharpstream_la_CFLAGS = $(CFLAGS)
libharpstream_la_SOURCES = stream.c ../../defs_h.c
libharpstream_la_LIBADD = ../libharpplugin.la

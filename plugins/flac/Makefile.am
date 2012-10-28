lib_LTLIBRARIES = libharpflac.la
libharpflac_la_CFLAGS = $(CFLAGS)
libharpflac_la_LDFLAGS = -module -avoid-version -lFLAC  $(SOUND_OUT)
libharpflac_la_SOURCES = flac.c ../../defs_h.c
libharpflac_la_LIBADD = ../libharpplugin.la
INCLUDES = -I../.. -I.. -I/usr/include/FLAC
pkgdata_DATA = libharpflac.sql

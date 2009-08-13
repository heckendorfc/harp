lib_LTLIBRARIES = libharpaac.la
libharpaac_la_CFLAGS = $(CFLAGS)
libharpaac_la_LDFLAGS = -module -avoid-version -lfaad -lmp4ff $(SOUND_OUT)
libharpaac_la_SOURCES = aac.c
INCLUDES = -I../.. -I..
pkgdata_DATA = libharpaac.sql

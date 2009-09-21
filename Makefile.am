ACLOCAL_AMFLAGS = -I m4
bin_PROGRAMS = harp
harp_SOURCES = harp.c
#harp_LDFLAGS = -Wl,-rpath,/usr/local/lib/
pkgdata_DATA = share/create.sql share/defaults.conf
man1_MANS = share/harp.1
AM_CFLAGS = -DSHARE_PATH="\"$(datadir)\"" -DLIB_PATH="\"$(libdir)\""
SUBDIRS = plugins

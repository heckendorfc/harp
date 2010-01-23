ACLOCAL_AMFLAGS = -I m4
bin_PROGRAMS = harp
harp_SOURCES = \
harp.c \
argparse.c \
harpconfig.c \
dbact.c \
insert.c \
edit.c \
util.c \
message.c \
player.c \
shuffle.c \
list.c \
admin.c \
portal.c \
tree.c \
harp.h \
argparse.h \
harpconfig.h \
dbact.h \
insert.h \
edit.h \
util.h \
message.h \
player.h \
shuffle.h \
list.h \
admin.h \
portal.h \
tree.h

#harp_LDFLAGS = -Wl,-rpath,/usr/local/lib/
pkgdata_DATA = share/create.sql share/defaults.conf
man1_MANS = share/harp.1
AM_CFLAGS = -DSHARE_PATH="\"$(datadir)\"" -DLIB_PATH="\"$(libdir)\""
SUBDIRS = . plugins

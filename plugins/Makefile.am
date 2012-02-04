noinst_LTLIBRARIES = libharpplugin.la
libharpplugin_la_LDFLAGS = -module -avoid-version $(SOUND_OUT)
libharpplugin_la_CFLAGS = -D_HARP_PLUGIN $(CFLAGS)
libharpplugin_la_SOURCES = plugin.c plugin.h ../defs.h ../message.c ../message.h ../sndutil.c ../sndutil.h
INCLUDES = -I. -I..
SUBDIRS = . 

if WITH_OSS
libharpplugin_la_SOURCES += ../ossutil.c ../ossutil.h
endif
if WITH_ALSA
libharpplugin_la_SOURCES += ../alsautil.c ../alsautil.h
endif
if WITH_JACK
libharpplugin_la_SOURCES += ../jackutil.c ../jackutil.h
endif

if WITH_MP3
SUBDIRS += mp3
endif
if WITH_AAC
SUBDIRS += aac
endif
if WITH_VORBIS
SUBDIRS += vorbis
endif
if WITH_FLAC
SUBDIRS += flac
endif
if WITH_STREAM
SUBDIRS += stream
endif

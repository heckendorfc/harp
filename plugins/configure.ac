#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(plugin.h)
AM_INIT_AUTOMAKE(harp/plugins, 0, 1)
AC_CONFIG_SRCDIR([mp3/mp3.c],[aac/aac.c],[vorbis/vorbis.c],[flac/flac.c])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

CFLAGS="$CFLAGS -I/usr/local/include"
LDFLAGS="$LDFLAGS -L/usr/local/lib"


AC_ARG_WITH([mp3],
[AS_HELP_STRING(--without-mp3, Disable mp3 support)],
[], [with_mp3=yes])

AC_ARG_WITH([aac],
[AS_HELP_STRING(--without-aac, Disable aac support)],
[], [with_aac=yes])

AC_ARG_WITH([vorbis],
[AS_HELP_STRING(--without-vorbis, Disable vorbis support)],
[], [with_vorbis=yes])

AC_ARG_WITH([flac],
[AS_HELP_STRING(--without-flac, Disable flac support)],
[], [with_flac=yes])


AC_ARG_WITH([alsa],
[AS_HELP_STRING(--with-alsa, Enable alsa support)],
[], [with_alsa=no])

AC_ARG_WITH([jack],
[AS_HELP_STRING(--with-jack, Enable jack support)],
[], [with_jack=no])

# Checks for libraries.
#AC_CHECK_LIB([asound], [snd_pcm_open])

AS_IF([test "$with_mp3" != no],
[AC_CHECK_LIB([mpg123], [mpg123_init],
	AC_CONFIG_FILES([mp3/Makefile]),
	[with_mp3=no])])

AS_IF([test "$with_aac" != no],
[AC_CHECK_LIB([faad], [NeAACDecOpen],
	[AC_CHECK_LIB([mp4ff], [mp4ff_open_read],
	AC_CONFIG_FILES([aac/Makefile]))],
	[with_aac=no])])

AS_IF([test "$with_vorbis" != no],
[AC_CHECK_LIB([vorbisfile], [ov_open_callbacks],
	AC_CONFIG_FILES([vorbis/Makefile]),
	[with_vorbis=no])])

AS_IF([test "$with_flac" != no],
[AC_CHECK_LIB([FLAC], [FLAC__stream_decoder_new],
	AC_CONFIG_FILES([flac/Makefile]),
	[with_flac=no])])


AS_IF([test "$with_alsa" != no],
[AC_CHECK_LIB([asound], [snd_pcm_open],
	[LIBS="$LIBS -lasound" AC_DEFINE([WITH_ALSA],[1],[alsa define])],
	[with_alsa=no])])

AS_IF([test "$with_jack" != no],
[AC_CHECK_LIB([jack], [jack_activate],
	[LIBS="$LIBS -ljack" AC_DEFINE([WITH_JACK],[1],[jack define])],
	[with_jack=no])])


AM_CONDITIONAL([WITH_MP3], [test "$with_mp3" != no])
AM_CONDITIONAL([WITH_AAC], [test "$with_aac" != no])
AM_CONDITIONAL([WITH_VORBIS], [test "$with_vorbis" != no])
AM_CONDITIONAL([WITH_FLAC], [test "$with_flac" != no])


# Checks for header files.
AC_DEFINE(_FILE_OFFSET_BITS,64)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

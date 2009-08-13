#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(plugin.h)
AM_INIT_AUTOMAKE(harp/plugins, 0, 1)
AC_CONFIG_SRCDIR([mp3/mp3.c],[aac/aac.c],[vorbis/vorbis.c],[flac/flac.c])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
#AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
#AC_PROG_RANLIB

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


# Checks for libraries.
AC_CHECK_LIB([asound], [snd_pcm_open])

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


AM_CONDITIONAL([WITH_MP3], [test "$with_mp3" != no])
AM_CONDITIONAL([WITH_AAC], [test "$with_aac" != no])
AM_CONDITIONAL([WITH_VORBIS], [test "$with_vorbis" != no])
AM_CONDITIONAL([WITH_FLAC], [test "$with_flac" != no])


# Checks for header files.
#AC_CHECK_HEADERS([stdlib.h string.h sys/ioctl.h termios.h unistd.h])
AC_DEFINE(_FILE_OFFSET_BITS,64)

# Checks for typedefs, structures, and compiler characteristics.
#AC_TYPE_SIZE_T
#AC_TYPE_SSIZE_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T

# Checks for library functions.
#AC_FUNC_ALLOCA
#AC_FUNC_MALLOC
#AC_CHECK_FUNCS([memmove pow])

AC_CONFIG_FILES([Makefile])
#AC_CONFIG_FILES([mp3/Makefile])
#AC_CONFIG_FILES([aac/Makefile])
#AC_CONFIG_FILES([vorbis/Makefile])

AC_OUTPUT

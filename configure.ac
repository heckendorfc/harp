#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(harp, 0.2.1)
AC_CONFIG_SRCDIR(harp.c)
AM_INIT_AUTOMAKE
#AC_INIT(harp.c)
#AM_INIT_AUTOMAKE(harp, 0, 1)
#LT_INIT(dlopen])
#AC_CONFIG_SRCDIR([harp.c])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

CFLAGS="$CFLAGS -I/usr/local/include"
LDFLAGS="$LDFLAGS -L/usr/local/lib"
# Checks for libraries.
AC_CHECK_LIB([asound], [snd_pcm_open])
#AC_SEARCH_LIBS([dlopen], [dl])
#AC_CHECK_LIB([faad], [NeAACDecOpen])
#AC_CHECK_LIB([mp4ff], [mp4ff_open_read])
#AC_CHECK_LIB([mpg123], [mpg123_init])
AC_CHECK_LIB([pthread], [pthread_create])
#AC_CHECK_LIB([sndutil], [snd_init])
AC_CHECK_LIB([sqlite3], [sqlite3_exec])
#AC_CHECK_LIB([vorbisfile], [ov_open_callbacks])

# Checks for header files.
#AC_CHECK_HEADERS([stdint.h stdlib.h string.h sys/ioctl.h termios.h unistd.h])
AC_CHECK_HEADERS([ftw.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_HEADER_STDBOOL
AC_C_INLINE
#AC_TYPE_INT16_T
#AC_TYPE_INT32_T
#AC_TYPE_INT64_T
#AC_TYPE_INT8_T
#AC_TYPE_SIZE_T
#AC_TYPE_SSIZE_T
#AC_TYPE_UINT16_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T
#AC_TYPE_UINT8_T

# Checks for library functions.
#AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
#AC_FUNC_MALLOC
#AC_FUNC_REALLOC
#AC_CHECK_FUNCS([memmove memset pow strchr strdup strtol])

AC_CONFIG_FILES([Makefile])
                 #libs/Makefile])
AC_OUTPUT

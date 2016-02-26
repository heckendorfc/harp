# Finds Sndio library
#
#  Sndio_INCLUDE_DIR - where to find faad.h, etc.
#  Sndio_LIBRARIES   - List of libraries when using Sndio.
#  Sndio_FOUND       - True if faad found.
#

if (Sndio_INCLUDE_DIR)
  # Already in cache, be silent
  set(Sndio_FIND_QUIETLY TRUE)
endif (Sndio_INCLUDE_DIR)

find_path(Sndio_INCLUDE_DIR sndio.h
  /usr/include
)

set(Sndio_NAMES sndio)
find_library(Sndio_LIBRARY
  NAMES ${Sndio_NAMES}
  HINTS
  PATHS
  /usr/lib
)

if (Sndio_INCLUDE_DIR AND Sndio_LIBRARY)
   set(Sndio_FOUND TRUE)
   set( Sndio_LIBRARIES ${Sndio_LIBRARY} )
else (Sndio_INCLUDE_DIR AND Sndio_LIBRARY)
   set(Sndio_FOUND FALSE)
   set(Sndio_LIBRARIES)
endif (Sndio_INCLUDE_DIR AND Sndio_LIBRARY)

if (Sndio_FOUND)
   if (NOT Sndio_FIND_QUIETLY)
      message(STATUS "Found Sndio: ${Sndio_LIBRARY}")
   endif (NOT Sndio_FIND_QUIETLY)
else (Sndio_FOUND)
   if (Sndio_FIND_REQUIRED)
      message(STATUS "Looked for Sndio libraries named ${Sndio_NAMES}.")
      message(STATUS "Include file detected: [${Sndio_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Sndio_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Sndio library")
   endif (Sndio_FIND_REQUIRED)
endif (Sndio_FOUND)

mark_as_advanced(
  Sndio_LIBRARY
  Sndio_INCLUDE_DIR
  )


cmake_minimum_required (VERSION 2.6)
project (harp)

add_definitions(-DPACKAGE_VERSION="0.6.0")
add_definitions(-DSHARE_PATH="${CMAKE_INSTALL_PREFIX}/share" )
add_definitions(-DLIB_PATH="${CMAKE_INSTALL_PREFIX}/lib" )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option (WITH_OSS "Use OSS for sound output.")
option (WITH_ALSA "Use ALSA for sound output.")
option (WITH_JACK "Use JACK for sound output.")
option (WITH_PULSE "Use PulseAudio for sound output.")

if (WITH_OSS)
	set (SOUND_SRC ${SOUND_SRC} ossutil.c)
	set (SOUND_LIB ${SOUND_LIB})
endif (WITH_OSS)
if (WITH_ALSA)
	add_definitions(-DWITH_ALSA=1)
	find_package(ALSA REQUIRED)
	include_directories(${ALSA_INCLUDE_DIRS})
	set (SOUND_SRC ${SOUND_SRC} alsautil.c)
	set (SOUND_LIB ${ALSA_LIBRARIES})
endif (WITH_ALSA)
if (WITH_JACK)
	set (SOUND_SRC ${SOUND_SRC} jackutil.c)
	set (SOUND_LIB ${SOUND_LIB})
endif (WITH_JACK)
if (WITH_PULSE)
	set (SOUND_SRC ${SOUND_SRC} pulseutil.c)
	set (SOUND_LIB ${SOUND_LIB})
endif (WITH_PULSE)

find_package(FTW)
if (FTW_FOUND)
	add_definitions(-DHAVE_FTW_H=1)
	include_directories(${FTW_INCLUDE_DIR})
endif (FTW_FOUND)

find_package(Sqlite REQUIRED)
include_directories(${SQLITE_INCLUDE_DIR})

add_subdirectory (plugins)

add_executable (harp ${SOUND_SRC}
harp.c
defs_h.c
argparse.c
harpconfig.c
dbact.c
insert.c
edit.c
util.c
message.c
player.c
shuffle.c
list.c
admin.c
portal.c
tree.c
sndutil.c
harp.h
argparse.h
harpconfig.h
dbact.h
insert.h
edit.h
util.h
message.h
player.h
shuffle.h
list.h
admin.h
portal.h
tree.h
sndutil.h)

target_link_libraries (harp ${SOUND_LIB} ${SQLITE_LIBRARIES})

FILE(GLOB shared_files share/*)
INSTALL(FILES README ${shared_files} DESTINATION share/harp)
INSTALL(TARGETS harp RUNTIME DESTINATION bin)
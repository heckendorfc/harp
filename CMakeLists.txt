cmake_minimum_required (VERSION 2.6)
project (harp)

#SET(CMAKE_C_FLAGS "-g -O0 -Wall")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option (WITH_OSS "Use OSS for sound output.")
option (WITH_ALSA "Use ALSA for sound output.")
option (WITH_JACK "Use JACK for sound output.")
option (WITH_PULSE "Use PulseAudio for sound output.")

if (WITH_OSS)
	add_definitions(-DWITH_OSS=1)
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
	find_package(Jack REQUIRED)
	find_package(Samplerate REQUIRED)
	include_directories(${JACK_INCLUDE_DIRS} ${SAMPLERATE_INCLUDE_DIR})
	add_definitions(-DWITH_JACK=1 -DRESAMPLE_QUALITY=0)
	set (SOUND_SRC ${SOUND_SRC} jackutil.c)
	set (SOUND_LIB ${SOUND_LIB} m ${JACK_LIBRARIES} ${SAMPLERATE_LIBRARIES})
endif (WITH_JACK)
if (WITH_PULSE)
	add_definitions(-DWITH_PULSE=1)
	#find_package(PulseAudio REQUIRED)
	find_package(LibPulseSimple REQUIRED)
	include_directories(${LibPulseSimple_INCLUDE_DIR})
	set (SOUND_SRC ${SOUND_SRC} pulseutil.c)
	set (SOUND_LIB ${SOUND_LIB} ${LibPulseSimple_LIBRARY})
endif (WITH_PULSE)

find_package(FTW)
if (FTW_FOUND)
	add_definitions(-DHAVE_FTW_H=1)
	include_directories(${FTW_INCLUDE_DIR})
endif (FTW_FOUND)


if (NOT DEFINED INSTALL_MANPREFIX)
	set(INSTALL_MANPREFIX ${CMAKE_INSTALL_PREFIX})
endif ()

if (NOT DEFINED INSTALL_LIBDIR)
	set(INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
endif ()

add_definitions(-DPACKAGE_VERSION="0.6.0")
add_definitions(-DSHARE_PATH="${CMAKE_INSTALL_PREFIX}/share" )
add_definitions(-DLIB_PATH="${INSTALL_LIBDIR}" )


find_package (Threads REQUIRED)
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
edit_shell.c
util.c
message.c
player.c
shuffle.c
list.c
admin.c
portal.c
tree.c
edit.tab.c
build.c
lex.c
lex_dfa.c
sndutil.c)

target_link_libraries (harp ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${SOUND_LIB} ${SQLITE_LIBRARIES})

#FILE(GLOB shared_files share/*)
SET(shared_files share/defaults.conf share/create.sql)
SET(doc_files README)
SET(man_files share/harp.1)
INSTALL(FILES ${shared_files} DESTINATION share/harp)
INSTALL(FILES ${man_files} DESTINATION ${INSTALL_MANPREFIX}/man/man1)
INSTALL(FILES ${doc_files} DESTINATION share/doc/harp)
INSTALL(TARGETS harp RUNTIME DESTINATION bin)

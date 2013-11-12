add_definitions(-D_HARP_PLUGIN=1)
include_directories(${CMAKE_SOURCE_DIR})
add_library(libharpplugin SHARED plugin.c ../message.c ../sndutil.c ../${SOUND_SRC})

option (WITH_MP3 "Enable MP3 support")

if (WITH_MP3)
	add_subdirectory(mp3)
endif(WITH_MP3)

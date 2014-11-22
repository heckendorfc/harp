add_definitions(-D_HARP_PLUGIN=1)
find_package(MPG123 REQUIRED)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins ${MPG123_INCLUDE_DIR})
link_directories(${CMAKE_SOURCE_DIR}/plugins)
add_library(harpmp3 MODULE mp3.c ../../defs_h.c)
target_link_libraries (harpmp3 harpplugin ${MPG123_LIBRARY} ${SOUND_LIB})

INSTALL(FILES libharpmp3.sql DESTINATION share/harp)
INSTALL(TARGETS harpmp3 LIBRARY DESTINATION ${INSTALL_LIBDIR})

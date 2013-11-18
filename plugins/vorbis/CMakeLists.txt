add_definitions(-D_HARP_PLUGIN=1)
find_package(Vorbis REQUIRED)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins ${VORBIS_INCLUDE_DIR})
link_directories(${CMAKE_SOURCE_DIR}/plugins)
add_library(harpvorbis SHARED vorbis.c ../../defs_h.c)
target_link_libraries (harpvorbis harpplugin ${VORBIS_LIBRARIES})

INSTALL(FILES libharpvorbis.sql DESTINATION share/harp)
INSTALL(TARGETS harpvorbis LIBRARY DESTINATION lib)

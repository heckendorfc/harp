add_definitions(-D_HARP_PLUGIN=1)
find_package(FLAC REQUIRED)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins ${FLAC_INCLUDE_DIR})
link_directories(${CMAKE_SOURCE_DIR}/plugins)
add_library(harpflac STATIC flac.c ../../defs_h.c)
target_link_libraries (harpflac harpplugin ${FLAC_LIBRARIES} ${SOUND_LIB})

#INSTALL(FILES libharpflac.sql DESTINATION share/harp)
#INSTALL(TARGETS harpflac LIBRARY DESTINATION ${INSTALL_LIBDIR})

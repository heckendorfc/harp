add_definitions(-D_HARP_PLUGIN=1)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins)
link_directories(${CMAKE_SOURCE_DIR}/plugins)
add_library(harpstream STATIC stream.c ../../defs_h.c)
target_link_libraries (harpstream harpplugin ${CMAKE_THREAD_LIBS_INIT})

#INSTALL(FILES libharpstream.sql DESTINATION share/harp)
#INSTALL(TARGETS harpstream LIBRARY DESTINATION ${INSTALL_LIBDIR})

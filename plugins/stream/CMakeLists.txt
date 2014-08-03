add_definitions(-D_HARP_PLUGIN=1)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins)
link_directories(${CMAKE_SOURCE_DIR}/plugins)
add_library(harpstream MODULE stream.c ../../defs_h.c)
target_link_libraries (harpstream harpplugin)

INSTALL(FILES libharpstream.sql DESTINATION share/harp)
INSTALL(TARGETS harpstream LIBRARY DESTINATION lib)

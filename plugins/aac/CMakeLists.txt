add_definitions(-D_HARP_PLUGIN=1)
find_package(Faad REQUIRED)
#add_subdirectory(mp4lib)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins ${Faad_INCLUDE_DIR} mp4lib)
link_directories(${CMAKE_SOURCE_DIR}/plugins)
add_library(harpaac STATIC aac.c ../../defs_h.c mp4lib/mp4lib.c)
target_link_libraries (harpaac harpplugin ${Faad_LIBRARIES} ${SOUND_LIB})

#INSTALL(FILES libharpaac.sql DESTINATION share/harp)
#INSTALL(TARGETS harpaac LIBRARY DESTINATION ${INSTALL_LIBDIR})

add_definitions(-D_HARP_PLUGIN=1)
find_package(Faad REQUIRED)
#TODO: find_package mp4v2 or ffmpeg
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins ${Faad_INCLUDE_DIR})
link_directories(${CMAKE_SOURCE_DIR}/plugins)
add_library(libharpaac SHARED aac.c ../../defs_h.c)
target_link_libraries (libharpaac libharpplugin ${Faad_LIBRARIES})


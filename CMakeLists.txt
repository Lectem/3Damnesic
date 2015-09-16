cmake_minimum_required(VERSION 2.8)
project(3DAmneSic C CXX ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(Tools3DS)

find_package(CTRULIB REQUIRED)
find_package(FFMPEG REQUIRED)

file(GLOB_RECURSE SOURCE_FILES
    source/*
    include/*
)

add_executable(3DAmneSic ${SOURCE_FILES})
target_link_libraries(3DAmneSic ${FFMPEG_swresample_LIBRARY} ${FFMPEG_swscale_LIBRARY} ${FFMPEG_avformat_LIBRARY} ${FFMPEG_avcodec_LIBRARY} ${FFMPEG_avutil_LIBRARY} m 3ds::ctrulib)
target_include_directories(3DAmneSic PUBLIC  ${CMAKE_CURRENT_LIST_DIR}/include ${FFMPEG_INCLUDE_DIR})
if(3DS)
    target_embed_shader(3DAmneSic data/shader.pica)
    add_3dsx_target(3DAmneSic 3DAmneSic "A WIP media player !" Lectem)
    add_netload_target(run 3DAmneSic)
endif()
cmake_minimum_required(VERSION 2.8)
project(videoPlayer C CXX ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(Tools3DS)

find_package(CTRULIB REQUIRED)
find_package(FFMPEG REQUIRED)

file(GLOB_RECURSE SOURCE_FILES
    source/*
    include/*
)

add_executable(videoPlayer ${SOURCE_FILES})
target_link_libraries(videoPlayer ${FFMPEG_swresample_LIBRARY} ${FFMPEG_swscale_LIBRARY} ${FFMPEG_avformat_LIBRARY} ${FFMPEG_avcodec_LIBRARY} ${FFMPEG_avutil_LIBRARY} m ${LIBCTRU_LIBRARIES})
target_include_directories(videoPlayer PUBLIC  ${CMAKE_CURRENT_LIST_DIR}/include ${FFMPEG_INCLUDE_DIR} ${LIBCTRU_INCLUDE_DIRS})
if(3DS)
    target_embed_shader(videoPlayer data/shader.pica)
    add_3dsx_target(videoPlayer 3DAmneSic "A WIP media player !" Lectem)
endif()

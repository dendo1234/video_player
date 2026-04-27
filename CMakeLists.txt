cmake_minimum_required(VERSION 3.26)
project(masvp VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
find_package(SDL3_ttf CONFIG REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/DejaVuSans)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/clay)

find_path(AVCODEC_INCLUDE_DIR NAMES libavcodec/avcodec.h REQUIRED)
find_library(AVCODEC_LIBRARY NAMES avcodec REQUIRED)

find_path(AVFORMAT_INCLUDE_DIR NAMES libavformat/avformat.h REQUIRED)
find_library(AVFORMAT_LIBRARY NAMES avformat REQUIRED)

find_path(AVUTIL_INCLUDE_DIR NAMES libavutil/avutil.h REQUIRED)
find_library(AVUTIL_LIBRARY NAMES avutil REQUIRED)

find_path(SWSCALE_INCLUDE_DIR NAMES libswscale/swscale.h REQUIRED)
find_library(SWSCALE_LIBRARY NAMES swscale REQUIRED)

find_path(SWRESAMPLE_INCLUDE_DIR NAMES libswresample/swresample.h REQUIRED)
find_library(SWRESAMPLE_LIBRARY NAMES swresample REQUIRED)



add_subdirectory(src)



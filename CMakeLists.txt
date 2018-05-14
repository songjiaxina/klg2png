cmake_minimum_required(VERSION 2.8)

project(klg2png)

find_package(OpenCV REQUIRED)
find_package(JPEG QUIET)
find_package(ZLIB REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS}  ${JPEG_INCLUDE_DIR} )
include_directories(${ZLIB_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS ${ADDITIONAL_CMAKE_CXX_FLAGS} "-O3 -msse2 -msse3 -Wall -std=c++11")


add_executable(klg2png main.cpp)

target_link_libraries(klg2png
                      ${OpenCV_LIBS} ${JPEG_LIBRARY} ${ZLIB_LIBRARY}
)



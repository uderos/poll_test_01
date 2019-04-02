cmake_minimum_required (VERSION 2.8.11)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++14 -lpthread")

SET(CMAKE_BUILD_TYPE Debug)
#SET(CMAKE_BUILD_TYPE Release)

include_directories(${CMAKE_SOURCE_DIR})
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/poll_lib)

project (poll01)

add_subdirectory (poll_lib)
add_subdirectory (test01)
add_subdirectory (test02)


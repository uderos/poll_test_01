cmake_minimum_required (VERSION 2.8.11)

set(SRC_FILES test01.cpp)

add_executable(test01 ${SRC_FILES})
TARGET_LINK_LIBRARIES(test01 poll_lib)


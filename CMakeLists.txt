cmake_minimum_required(VERSION 3.9.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#include("cmake/util.cmake")
include("cmake/CPM.cmake")

project(miplib)

add_compile_options(
  "$<$<CONFIG:DEBUG>:-g3;-ggdb>"
)

add_subdirectory(${CMAKE_SOURCE_DIR}/miplib)

enable_testing()
add_subdirectory(test)

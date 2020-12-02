cmake_minimum_required(VERSION 3.9.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include("cmake/HunterGate.cmake")
include("cmake/util.cmake")

HunterGate(
  URL "https://github.com/cpp-pm/hunter/archive/v0.23.284.tar.gz"
  SHA1 "b15e14b31d96ee2e2be32187d21e2390d0b974c8"
)

project(miplib)

add_compile_options(
  "$<$<CONFIG:DEBUG>:-g3;-ggdb>"
)

add_subdirectory(${CMAKE_SOURCE_DIR}/miplib)

enable_testing()
add_subdirectory(test)

cmake_minimum_required(VERSION 3.9.2)

# Define the name of our project
project(test)

add_executable(unit_test
  unit/expr.cpp
  unit/main.cpp
)

set(COMPILE_FLAGS -Werror -Wall -Wextra)

target_compile_options(unit_test PRIVATE ${COMPILE_FLAGS})

include("../cmake/CPM.cmake")

CPMAddPackage("https://github.com/catchorg/Catch2.git@2.13.3")

target_link_libraries(unit_test Catch2::Catch2)

target_link_libraries(
  unit_test miplib
)

add_test(NAME unit_test COMMAND unit_test)

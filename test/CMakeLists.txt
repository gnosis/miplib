cmake_minimum_required(VERSION 3.9.2)

# Define the name of our project
project(test)

add_executable(unit_test
  unit/expr.cpp
  unit/main.cpp
)

set(COMPILE_FLAGS -Werror -Wall -Wextra)

target_compile_options(unit_test PRIVATE ${COMPILE_FLAGS})


include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.3
)
FetchContent_MakeAvailable(Catch2)

target_link_libraries(unit_test Catch2::Catch2)

target_link_libraries(
  unit_test miplib
)

add_test(NAME unit_test COMMAND unit_test)

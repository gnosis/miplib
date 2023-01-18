cmake_minimum_required(VERSION 3.9.2)

project(miplib)

include("../cmake/CPM.cmake")

CPMAddPackage("https://github.com/gabime/spdlog.git@1.8.5")
CPMAddPackage("https://github.com/fmtlib/fmt.git#7.1.3")

set(SOURCE_FILES 
  util/scale.cpp
  expr.cpp
  var.cpp
  constr.cpp
  solver.cpp
  capi.cpp
)

# Optional dependency: Gurobi.
find_path(GUROBI_INCLUDE_DIR
  NAMES gurobi_c++.h
  PATHS "${GUROBI}/include"
)

find_library(GUROBI_C_LIBRARY
  NAMES gurobi
  gurobi95
  libgurobi95.so
  gurobi91
  libgurobi91.so
  PATHS "${GUROBI}/lib"
)

find_library(GUROBI_CPP_LIBRARY
  NAMES gurobi_c++
  PATHS "${GUROBI}/lib"
)

set(LIBS fmt spdlog)

if (GUROBI_INCLUDE_DIR AND GUROBI_C_LIBRARY AND GUROBI_CPP_LIBRARY)

include_directories(${GUROBI_INCLUDE_DIR})
set(SOURCE_FILES ${SOURCE_FILES} gurobi/solver.cpp gurobi/var.cpp)
set(LIBS ${LIBS} ${GUROBI_CPP_LIBRARY} ${GUROBI_C_LIBRARY})
add_definitions(-DWITH_GUROBI)

message("-- Gurobi backend: enabled!")

else (GUROBI_INCLUDE_DIR AND GUROBI_C_LIBRARY AND GUROBI_CPP_LIBRARY)

message(WARNING "Could not find Gurobi: the backend will be disabled. Did you set GUROBI env var?")

endif (GUROBI_INCLUDE_DIR AND GUROBI_C_LIBRARY AND GUROBI_CPP_LIBRARY)

# Optional dependency: SCIP.

find_path(SCIP_INCLUDE_DIR
  NAMES scip/scip.h
  PATHS "${SCIP}/include/"
)

find_library(SCIP_C_LIBRARY
  NAMES scip
  PATHS "${SCIP}/lib"
)

if (SCIP_INCLUDE_DIR AND SCIP_C_LIBRARY)

include_directories(${SCIP_INCLUDE_DIR})

set(SOURCE_FILES ${SOURCE_FILES} scip/solver.cpp scip/var.cpp)
# Compiling SCIP headers issue a lot of unused parameters. Disabling those warnings for these files.
#set_source_files_properties(scip/solver.cpp scip/var.cpp PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
set(LIBS ${LIBS} ${SCIP_C_LIBRARY})
add_definitions(-DWITH_SCIP)

message("-- SCIP backend: enabled!")

else (SCIP_INCLUDE_DIR AND SCIP_C_LIBRARY)

message(WARNING "Could not find SCIP: the backend will be disabled. Did you set SCIP env var?")

endif (SCIP_INCLUDE_DIR AND SCIP_C_LIBRARY)

# Optional dependency: LPSolve.

find_path(lpsolve_INCLUDE_DIR
  NAMES lpsolve/lp_lib.h
  PATHS "${lpsolve}/include/"
)

find_library(lpsolve_C_LIBRARY
  NAMES lpsolve55
  PATHS "${lpsolve}/lib/lp_solve"
)

if (lpsolve_INCLUDE_DIR AND lpsolve_C_LIBRARY)

include_directories(${lpsolve_INCLUDE_DIR})

set(SOURCE_FILES ${SOURCE_FILES} lpsolve/solver.cpp lpsolve/var.cpp lpsolve/util.cpp)
set(LIBS ${LIBS} ${lpsolve_C_LIBRARY} ${CMAKE_DL_LIBS} colamd)
add_definitions(-DWITH_LPSOLVE)

message("-- lpsolve backend: enabled!")

else (lpsolve_INCLUDE_DIR AND lpsolve_C_LIBRARY)

message(WARNING "Could not find lpsolve: the backend will be disabled.")

endif (lpsolve_INCLUDE_DIR AND lpsolve_C_LIBRARY)

# Define the output library to be a static file (.a) and output it as "(lib)miplib.a"
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC" )

set(COMPILE_FLAGS -Werror -Wall -Wno-sign-compare -Wextra)

target_compile_options(${PROJECT_NAME} PUBLIC ${COMPILE_FLAGS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

find_package(Boost CONFIG REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${LIBS})

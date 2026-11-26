cmake_minimum_required(VERSION 3.16)
project(rtsim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(RTSIM_GUI "Build the interactive viewer (raylib)" ON)
option(RTSIM_TESTS "Build the unit tests" ON)

# Simulation core: no graphics dependency at all.
add_library(rtsim_core STATIC
  src/core/Model.cpp
  src/core/Parser.cpp
  src/core/Simulator.cpp
  src/core/Analysis.cpp
  src/core/Report.cpp
  src/render/Layout.cpp
  src/render/SvgWriter.cpp)
target_include_directories(rtsim_core PUBLIC src)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(rtsim_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(rtsim src/main.cpp)
target_link_libraries(rtsim PRIVATE rtsim_core)

if(RTSIM_GUI)
  # Use the system raylib (>= 5.0) if installed, otherwise download 5.5.
  find_package(raylib 5.0 QUIET)
  if(NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
      GIT_REPOSITORY https://github.com/raysan5/raylib.git
      GIT_TAG 5.5
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(raylib)
  endif()
  target_sources(rtsim PRIVATE src/viewer/Viewer.cpp)
  target_compile_definitions(rtsim PRIVATE RTSIM_WITH_GUI)
  target_link_libraries(rtsim PRIVATE raylib)
endif()

if(RTSIM_TESTS)
  enable_testing()
  add_executable(rtsim_tests tests/test_core.cpp)
  target_link_libraries(rtsim_tests PRIVATE rtsim_core)
  add_test(NAME core COMMAND rtsim_tests)
endif()

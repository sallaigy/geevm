cmake_minimum_required(VERSION 3.28)
project(geevm)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

file(GLOB_RECURSE
  GEEVM_COMMON_SOURCES
  src/common/*.cpp
)

file(GLOB_RECURSE
  GEEVM_CLASS_FILE_SOURCES
  src/class_file/*.cpp
)

set(GEEVM_SOURCES
  ${GEEVM_COMMON_SOURCES}
  ${GEEVM_CLASS_FILE_SOURCES}
)

add_library(geevm ${GEEVM_SOURCES})

target_include_directories(geevm PUBLIC src)

add_subdirectory(src/unit_tests)

cmake_minimum_required(VERSION 3.28)
project(geevm)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

file(GLOB_RECURSE
  GEEVM_COMMON_SOURCES
  src/common/*.cpp
)

file(GLOB_RECURSE
  GEEVM_CLASS_FILE_SOURCES
  src/class_file/*.cpp
)

file(GLOB_RECURSE
  GEEVM_VM_SOURCES
  src/vm/*.cpp
)

set(GEEVM_SOURCES
  ${GEEVM_COMMON_SOURCES}
  ${GEEVM_CLASS_FILE_SOURCES}
  ${GEEVM_VM_SOURCES}
)

add_library(geevm ${GEEVM_SOURCES})

target_include_directories(geevm PUBLIC src)

add_subdirectory(src/unit_tests)

add_executable(java src/bin/java.cpp)
target_link_libraries(java geevm)
target_include_directories(java PUBLIC src)

add_subdirectory(tests/programs)

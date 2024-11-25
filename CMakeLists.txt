cmake_minimum_required(VERSION 3.28)
project(geevm)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

find_package(libzip CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter)
find_package(Java 1.8 EXACT REQUIRED)

get_filename_component(REAL_JAVAC ${Java_JAVAC_EXECUTABLE} REALPATH)
cmake_path(GET REAL_JAVAC PARENT_PATH JAVA_BIN_DIR)
cmake_path(GET JAVA_BIN_DIR PARENT_PATH JAVA_HOME)

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

include(CTest)
enable_testing()
add_subdirectory(src/unit_tests)
add_subdirectory(tests)

target_include_directories(geevm PUBLIC src)
target_link_libraries(geevm libzip::zip)

add_executable(java src/bin/java.cpp)
target_link_libraries(java geevm)
target_include_directories(java PUBLIC src)

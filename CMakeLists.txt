cmake_minimum_required(VERSION 3.28)
project(geevm)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

find_package(libzip CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter)
find_package(Java 17 EXACT REQUIRED)
find_package(JNI 17 EXACT REQUIRED)

get_filename_component(REAL_JAVAC ${Java_JAVAC_EXECUTABLE} REALPATH)
cmake_path(GET REAL_JAVAC PARENT_PATH JAVA_BIN_DIR)
cmake_path(GET JAVA_BIN_DIR PARENT_PATH JAVA_HOME)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/lib/modules)
  execute_process(COMMAND ${JAVA_BIN_DIR}/jimage extract --dir=${CMAKE_BINARY_DIR}/lib/modules ${JAVA_HOME}/lib/modules)
endif ()

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
  src/vm/jit/*.cpp
)

set(GEEVM_SOURCES
  ${GEEVM_COMMON_SOURCES}
  ${GEEVM_CLASS_FILE_SOURCES}
  ${GEEVM_VM_SOURCES}
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(HARDENED_COMPILER_FLAGS "-D_GLIBCXX_ASSERTIONS -Wall -Wextra -Wconversion -Wimplicit-fallthrough")
else ()
  set(HARDENED_COMPILER_FLAGS "")
endif ()

FetchContent_Declare(asmjit
  GIT_REPOSITORY "https://github.com/asmjit/asmjit"
  GIT_HASH "4111caeca42cb092669d27165eec1fd958ffe9a6"
)
FetchContent_MakeAvailable(asmjit)
target_compile_definitions(asmjit PUBLIC ASMJIT_EMBED)

add_library(geevm ${GEEVM_SOURCES})
target_compile_options(geevm PUBLIC ${HARDENED_COMPILER_FLAGS})

file(GLOB_RECURSE
  GEEVM_NATIVE_SOURCES
  src/native/**/*.cpp
)

add_library(geevm-libjava SHARED ${GEEVM_NATIVE_SOURCES})
target_include_directories(geevm-libjava PUBLIC ${JNI_INCLUDE_DIRS})
target_include_directories(geevm-libjava PUBLIC src)

include(CTest)
enable_testing()
add_subdirectory(src/unit_tests)
add_subdirectory(tests)

target_include_directories(geevm PUBLIC src)
target_include_directories(geevm PUBLIC ${JNI_INCLUDE_DIRS})
target_include_directories(geevm PUBLIC "{asmjit_SOURCE_DIR}/src")
target_link_libraries(geevm PUBLIC libzip::zip)
target_link_libraries(geevm PUBLIC ffi)
target_link_libraries(geevm PUBLIC asmjit::asmjit)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY "https://github.com/p-ranav/argparse"
  GIT_TAG v3.1
)
FetchContent_MakeAvailable(argparse)

add_executable(java src/bin/java.cpp)
target_link_libraries(java geevm)
target_link_libraries(java geevm-libjava)
target_link_options(java PRIVATE "-Wl,--no-as-needed")
target_include_directories(java PUBLIC src)
target_include_directories(java PRIVATE "${argparse_SOURCE_DIR}/include")

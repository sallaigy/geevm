# Load GoogleTest
FetchContent_Declare(
  gtest
  GIT_REPOSITORY "https://github.com/google/googletest"
  GIT_TAG release-1.11.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gtest)
include(GoogleTest)

set(TEST_SOURCES ClassFileReaderTest.cpp DescriptorTest.cpp GarbageCollectorTest.cpp)
add_executable(geevm_test ${TEST_SOURCES})

set(GEEVM_UNIT_TEST_FIXTURES_DIR ${CMAKE_SOURCE_DIR}/tests)

target_compile_definitions(geevm_test PRIVATE GEEVM_UNIT_TEST_FIXTURES_DIR="${GEEVM_UNIT_TEST_FIXTURES_DIR}")
target_include_directories(geevm_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(geevm_test gmock_main geevm)

gtest_discover_tests(geevm_test)

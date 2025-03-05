#ifndef GEEVM_UNIT_TEST_BASETEST_H
#define GEEVM_UNIT_TEST_BASETEST_H

#ifndef GEEVM_UNIT_TEST_FIXTURES_DIR
#error "GEEVM_UNIT_TEST_FIXTURES_DIR must be defined"
#endif

#include <filesystem>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

namespace geevm::testing
{

class BaseTest : public ::testing::Test
{
protected:
  static constexpr std::string_view FixturesPath = GEEVM_UNIT_TEST_FIXTURES_DIR;
  static constexpr std::string_view Jdk17Path = GEEVM_UNIT_TEST_JDK17_PATH;

  fs::path getResource(std::string_view path) const
  {
    return fs::path{FixturesPath}.append(path);
  }
};

} // namespace geevm::testing

#endif // GEEVM_UNIT_TEST_BASETEST_H

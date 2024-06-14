#ifndef GEEVM_UNIT_TEST_BASETEST_H
#define GEEVM_UNIT_TEST_BASETEST_H

#ifndef GEEVM_UNIT_TEST_FIXTURES_DIR
#error "GEEVM_UNIT_TEST_FIXTURES_DIR must be defined"
#endif

#include <gtest/gtest.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace geevm::testing
{

class BaseTest : public ::testing::Test
{
  static constexpr std::string_view FixturesPath = GEEVM_UNIT_TEST_FIXTURES_DIR;

protected:
  fs::path getResource(std::string_view path) const
  {
    return fs::path{FixturesPath}.append(path);
  }

private:
};

}

#endif //GEEVM_UNIT_TEST_BASETEST_H

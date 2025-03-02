#include "common/System.h"

#include <filesystem>

using namespace geevm;

std::optional<std::filesystem::path> geevm::findProgramLocation(std::string_view argvZero)
{
#ifdef __linux__
  {
    std::filesystem::path selfExecPath("/proc/self/exe");
    std::error_code errorCode;

    auto result = std::filesystem::read_symlink(selfExecPath, errorCode);
    if (!errorCode) {
      return result;
    }
  }
#endif

  if (argvZero.empty()) {
    return std::nullopt;
  }

  std::filesystem::path path(argvZero);
  std::error_code errorCode;

  auto canonical = std::filesystem::canonical(path, errorCode);
  if (!errorCode) {
    return canonical;
  }

  return std::nullopt;
}

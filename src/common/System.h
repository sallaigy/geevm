#ifndef GEEVM_COMMON_SYSTEM_H
#define GEEVM_COMMON_SYSTEM_H

#include <filesystem>
#include <optional>
#include <string>

namespace geevm
{

std::optional<std::filesystem::path> findProgramLocation(std::string_view argvZero);

}

#endif // GEEVM_COMMON_SYSTEM_H

#include "vm/ClassPath.h"

#include <algorithm>
#include <filesystem>

using namespace geevm;

namespace fs = std::filesystem;

static std::filesystem::path classNameToPath(types::JStringRef name)
{
  // Replace dot with slash
  types::JString pathStr(name);
  std::ranges::replace(pathStr, u'.', u'/');

  auto path = fs::path{pathStr + u".class"};

  return path;
}

void ClassPath::addJar(const std::string& path)
{
  mEntries.emplace_back(ZipArchive::open(path));
}

void ClassPath::addDirectory(const std::string& path)
{
  mEntries.emplace_back(path);
}

std::optional<ClassLocation> ClassPath::search(const types::JString& name)
{
  for (auto& entry : mEntries) {
    if (auto res = entry.search(name); res.has_value()) {
      return res;
    }
  }
  return std::nullopt;
}

std::optional<ZipArchive*> ClassPath::Entry::asJar()
{
  if (std::holds_alternative<std::unique_ptr<ZipArchive>>(mStorage)) {
    return std::get<std::unique_ptr<ZipArchive>>(mStorage).get();
  }
  return std::nullopt;
}

std::optional<std::string> ClassPath::Entry::asDirectory()
{
  if (std::holds_alternative<std::string>(mStorage)) {
    return std::get<std::string>(mStorage);
  }
  return std::nullopt;
}

std::optional<ClassLocation> ClassPath::Entry::search(const types::JString& name)
{
  auto path = classNameToPath(name);
  if (auto jar = this->asJar(); jar.has_value() && (*jar)->containsFile(path.filename())) {
    return ClassLocation::createFileLocation(path.filename());
  }

  if (auto directory = this->asDirectory(); directory.has_value()) {
    auto fullPath = fs::path{*directory} / path;
    if (fs::exists(fullPath)) {
      return ClassLocation::createFileLocation(fullPath);
    }
  }

  return std::nullopt;
}


#include "vm/ClassLoader.h"

#include <algorithm>
#include <filesystem>
#include <utility>

using namespace geevm;

JvmExpected<JClass*> ClassLoader::loadClass(const types::JString& name)
{
  if (auto it = mClasses.find(name); it != mClasses.end()) {
    return it->second.get();
  }

  auto location = findClassLocation(name);
  if (!location) {
    return makeError<JClass*, NoClassDefFoundError>();
  }

  if (location->isFile()) {
    auto classFile = ClassFile::fromFile(location->location());
    if (!classFile) {
      // TODO: Check exactly why the file cannot be loaded
      return makeError<JClass*, NoClassDefFoundError>();
    }

    auto r = mClasses.emplace(name, std::make_unique<JClass>(std::move(classFile)));
    return r.first->second.get();
  }

  return nullptr;
}

std::optional<ClassLocation> ClassLoader::findClassLocation(types::JStringRef name) const
{
  // Replace dot with slash
  types::JString path(name);
  std::ranges::replace(path, u'.', u'/');

  namespace fs = std::filesystem;
  auto classPath = fs::path{path + u".class"};

  if (fs::exists(classPath)) {
    return std::make_optional<>(ClassLocation{ClassLocation::Kind::File, classPath.string()});
  }

  return std::nullopt;
}

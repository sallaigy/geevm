#include "vm/ClassLoader.h"
#include "common/Zip.h"

#include "Vm.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <utility>

using namespace geevm;

static std::filesystem::path classNameToPath(types::JStringRef name)
{
  // Replace dot with slash
  types::JString pathStr(name);
  std::ranges::replace(pathStr, u'.', u'/');

  namespace fs = std::filesystem;
  auto path = fs::path{pathStr + u".class"};

  return path;
}

JvmExpected<JClass*> BootstrapClassLoader::loadClass(const types::JString& name)
{
  if (auto it = mClasses.find(name); it != mClasses.end()) {
    return it->second.get();
  }

  if (name.starts_with(u"[")) {
    return this->loadArrayClass(name);
  }

  JvmExpected<std::unique_ptr<InstanceClass>> loadResult;
  if (name.starts_with(u"java/") || name.starts_with(u"sun/") || name.starts_with(u"jdk/")) {
    if (mBootstrapArchive == nullptr) {
      mBootstrapArchive = ZipArchive::open(std::getenv("RT_JAR_PATH"));
    }

    if (mBootstrapArchive == nullptr) {
      return makeError<JClass*>(u"java/lang/NoClassDefFoundError");
    }

    ClassLocation location = ClassLocation::createJarLocation(mBootstrapArchive.get(), classNameToPath(name));
    loadResult = location.resolve();
  } else {
    loadResult = mNextClassLoader->loadClass(name);
  }

  if (!loadResult) {
    return makeError<InstanceClass*>(std::move(loadResult.error()));
  }

  auto [result, _] = mClasses.try_emplace(name, std::move(*loadResult));
  auto* klass = result->second->asInstanceClass();

  klass->initializeRuntimeConstantPool(mVm.heap().stringHeap(), *this);
  klass->prepare(*this, mVm.heap());

  return result->second.get();
}

JvmExpected<ArrayClass*> BootstrapClassLoader::loadArrayClass(const types::JString& name)
{
  auto arrayType = FieldType::parse(name);
  assert(arrayType.has_value() && "An array class descriptor should be parseable!");

  if (auto referenceType = arrayType->asObjectName(); referenceType.has_value()) {
    auto elementClass = this->loadClass(*referenceType);
    if (!elementClass) {
      return makeError<ArrayClass*>(std::move(elementClass.error()));
    }
  }

  auto [result, _] = mClasses.try_emplace(name, std::make_unique<ArrayClass>(name, *arrayType));
  auto* klass = result->second->asArrayClass();

  klass->prepare(*this, mVm.heap());

  return klass;
}

JvmExpected<std::unique_ptr<InstanceClass>> BaseClassLoader::loadClass(const types::JString& name)
{
  // TODO: Load JARs
  return ClassLocation::createFileLocation(classNameToPath(name)).resolve();
}

ClassLocation ClassLocation::createJarLocation(ZipArchive* archive, const std::string& fileName)
{
  return ClassLocation(std::make_pair(archive, fileName));
}

ClassLocation ClassLocation::createFileLocation(std::string fileName)
{
  return ClassLocation(fileName);
}

JvmExpected<std::unique_ptr<InstanceClass>> ClassLocation::resolve()
{
  if (std::holds_alternative<std::string>(mStorage)) {
    auto& fileName = std::get<std::string>(mStorage);
    auto classFile = ClassFile::fromFile(fileName);
    if (!classFile) {
      // TODO: Check exactly why the file cannot be loaded
      return makeError<std::unique_ptr<InstanceClass>>(u"java/lang/NoClassDefFoundError");
    }
    return std::make_unique<InstanceClass>(std::move(classFile));
  }

  auto& [zip, fileName] = std::get<std::pair<ZipArchive*, std::string>>(mStorage);
  if (zip == nullptr) {
    return makeError<std::unique_ptr<InstanceClass>>(u"java/lang/NoClassDefFoundError");
  }

  char* buffer;
  size_t size;
  if (!zip->readAsBinary(fileName, &buffer, &size)) {
    return makeError<std::unique_ptr<InstanceClass>>(u"java/lang/NoClassDefFoundError");
  }

  auto classFile = ClassFile::fromBytes(buffer, size);

  delete[] buffer;

  return std::make_unique<InstanceClass>(std::move(classFile));
}

BootstrapClassLoader::BootstrapClassLoader(Vm& vm)
  : mVm(vm), mNextClassLoader(std::make_unique<BaseClassLoader>())
{
}

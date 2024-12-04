#include "vm/ClassLoader.h"
#include "common/Zip.h"

#include "Vm.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <utility>

using namespace geevm;

JvmExpected<JClass*> BootstrapClassLoader::loadClass(const types::JString& name)
{
  if (auto it = mClasses.find(name); it != mClasses.end()) {
    return it->second.get();
  }

  if (name.starts_with(u"[")) {
    return this->loadArrayClass(name);
  }

  JvmExpected<std::unique_ptr<InstanceClass>> loadResult;
  std::optional<ClassLocation> location = mClassPath.search(name);

  if (location.has_value()) {
    loadResult = location->resolve();
  } else {
    for (const auto& classLoader : mClassLoaders) {
      loadResult = classLoader->loadClass(name);
      if (loadResult.has_value() && *loadResult != nullptr) {
        break;
      }
    }
  }

  if (!loadResult) {
    return makeError<InstanceClass*>(loadResult.error());
  }

  auto [result, _] = mClasses.try_emplace(name, std::move(*loadResult));
  auto* klass = result->second->asInstanceClass();

  klass->initializeRuntimeConstantPool(mVm.heap().stringHeap(), *this);
  klass->prepare(*this, mVm.heap());

  return result->second.get();
}

JvmExpected<JClass*> BootstrapClassLoader::loadUnpreparedClass(const types::JString& name)
{
  if (auto it = mClasses.find(name); it != mClasses.end()) {
    return it->second.get();
  }

  std::optional<ClassLocation> location = mClassPath.search(name);
  assert(location.has_value());
  auto loadResult = location->resolve();
  assert(loadResult.has_value());

  auto [result, _] = mClasses.try_emplace(name, std::move(*loadResult));
  auto* klass = result->second->asInstanceClass();

  klass->initializeRuntimeConstantPool(mVm.heap().stringHeap(), *this);

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

void BootstrapClassLoader::registerClassLoader(std::unique_ptr<ClassLoader> classLoader)
{
  mClassLoaders.emplace_back(std::move(classLoader));
}

JvmExpected<std::unique_ptr<InstanceClass>> BaseClassLoader::loadClass(const types::JString& name)
{
  if (auto location = mClassPath.search(name); location) {
    return location->resolve();
  }

  return nullptr;
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
  : mVm(vm)
{
}

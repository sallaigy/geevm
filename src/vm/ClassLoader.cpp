#include "vm/ClassLoader.h"

#include "Vm.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <utility>
#include <zip.h>

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
    // TODO: replace
    JarLocation location{std::getenv("RT_JAR_PATH"), classNameToPath(name)};
    loadResult = location.resolve();
  } else {
    loadResult = mNextClassLoader->loadClass(name);
  }

  if (!loadResult) {
    return makeError<InstanceClass*>(std::move(loadResult.error()));
  }

  auto [result, _] = mClasses.try_emplace(name, std::move(*loadResult));
  auto* klass = result->second->asInstanceClass();

  klass->initializeRuntimeConstantPool(mVm.internedStrings(), *this);
  klass->prepare(*this);
  klass->initialize(mVm);

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

  klass->prepare(*this);
  klass->initialize(mVm);

  return klass;
}

JvmExpected<std::unique_ptr<InstanceClass>> BaseClassLoader::loadClass(const types::JString& name)
{
  return ClassFileLocation{classNameToPath(name)}.resolve();
}

JvmExpected<std::unique_ptr<InstanceClass>> ClassFileLocation::resolve()
{
  auto classFile = ClassFile::fromFile(mFileName);
  if (!classFile) {
    // TODO: Check exactly why the file cannot be loaded
    return makeError<std::unique_ptr<InstanceClass>, NoClassDefFoundError>();
  }
  return std::make_unique<InstanceClass>(std::move(classFile));
}

JvmExpected<std::unique_ptr<InstanceClass>> JarLocation::resolve()
{
  int ec;
  zip_t* archive = zip_open(mArchiveLocation.c_str(), ZIP_RDONLY, &ec);
  if (archive == nullptr) {
    return makeError<std::unique_ptr<InstanceClass>, NoClassDefFoundError>();
  }

  int64_t fileIndex = zip_name_locate(archive, mFileName.c_str(), ZIP_FL_ENC_GUESS);
  if (fileIndex == -1) {
    zip_close(archive);
    return makeError<std::unique_ptr<InstanceClass>, NoClassDefFoundError>();
  }

  zip_stat_t stat;
  if (zip_stat_index(archive, fileIndex, ZIP_STAT_SIZE, &stat) != 0) {
    zip_close(archive);
    return makeError<std::unique_ptr<InstanceClass>, NoClassDefFoundError>();
  }

  zip_file_t* zipFile = zip_fopen_index(archive, fileIndex, 0);
  if (zipFile == nullptr) {
    zip_close(archive);
    return makeError<std::unique_ptr<InstanceClass>, NoClassDefFoundError>();
  }

  auto buffer = new char[stat.size];
  if (zip_fread(zipFile, buffer, stat.size) == -1) {
    zip_fclose(zipFile);
    zip_close(archive);
    delete[] buffer;
    return makeError<std::unique_ptr<InstanceClass>, NoClassDefFoundError>();
  }

  auto classFile = ClassFile::fromBytes(buffer, stat.size);

  zip_fclose(zipFile);
  zip_close(archive);
  delete[] buffer;

  return std::make_unique<InstanceClass>(std::move(classFile));
}

BootstrapClassLoader::BootstrapClassLoader(Vm& vm)
  : mVm(vm), mNextClassLoader(std::make_unique<BaseClassLoader>())
{
}

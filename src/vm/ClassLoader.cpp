#include "vm/ClassLoader.h"

#include <algorithm>
#include <filesystem>
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

  JvmExpected<std::unique_ptr<JClass>> klass;
  if (name.starts_with(u"java/lang")) {
    // TODO: replace
    JarLocation location{std::getenv("RT_JAR_PATH"), classNameToPath(name)};
    klass = location.resolve();
  } else {
    klass = mNextClassLoader->loadClass(name);
  }

  if (!klass) {
    return makeError<JClass*>(std::move(klass.error()));
  }

  auto [result, _] = mClasses.try_emplace(name, std::move(*klass));
  return result->second.get();
}

JvmExpected<std::unique_ptr<JClass>> BaseClassLoader::loadClass(const types::JString& name)
{
  return ClassFileLocation{classNameToPath(name)}.resolve();
}

JvmExpected<std::unique_ptr<JClass>> ClassFileLocation::resolve()
{
  auto classFile = ClassFile::fromFile(mFileName);
  if (!classFile) {
    // TODO: Check exactly why the file cannot be loaded
    return makeError<std::unique_ptr<JClass>, NoClassDefFoundError>();
  }
  return std::make_unique<JClass>(std::move(classFile));
}

JvmExpected<std::unique_ptr<JClass>> JarLocation::resolve()
{
  int ec;
  zip_t* archive = zip_open(mArchiveLocation.c_str(), ZIP_RDONLY, &ec);
  if (archive == nullptr) {
    return makeError<std::unique_ptr<JClass>, NoClassDefFoundError>();
  }

  int64_t fileIndex = zip_name_locate(archive, mFileName.c_str(), ZIP_FL_ENC_GUESS);
  if (fileIndex == -1) {
    zip_close(archive);
    return makeError<std::unique_ptr<JClass>, NoClassDefFoundError>();
  }

  zip_stat_t stat;
  if (zip_stat_index(archive, fileIndex, ZIP_STAT_SIZE, &stat) != 0) {
    zip_close(archive);
    return makeError<std::unique_ptr<JClass>, NoClassDefFoundError>();
  }

  zip_file_t* zipFile = zip_fopen_index(archive, fileIndex, 0);
  if (zipFile == nullptr) {
    zip_close(archive);
    return makeError<std::unique_ptr<JClass>, NoClassDefFoundError>();
  }

  auto buffer = new char[stat.size];
  if (zip_fread(zipFile, buffer, stat.size) == -1) {
    zip_fclose(zipFile);
    zip_close(archive);
    delete[] buffer;
    return makeError<std::unique_ptr<JClass>, NoClassDefFoundError>();
  }

  auto classFile = ClassFile::fromBytes(buffer, stat.size);

  zip_fclose(zipFile);
  zip_close(archive);
  delete[] buffer;

  return std::make_unique<JClass>(std::move(classFile));
}

BootstrapClassLoader::BootstrapClassLoader()
  : mNextClassLoader(std::make_unique<BaseClassLoader>())
{
}

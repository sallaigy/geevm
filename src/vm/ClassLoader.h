#ifndef GEEVM_VM_CLASSLOADER_H
#define GEEVM_VM_CLASSLOADER_H

#include <utility>

#include "common/JvmError.h"
#include "common/JvmTypes.h"
#include "vm/Class.h"

namespace geevm
{

class ClassLocation
{
public:
  virtual JvmExpected<std::unique_ptr<JClass>> resolve() = 0;
  virtual ~ClassLocation() = default;
};

class ClassFileLocation : public ClassLocation
{
public:
  explicit ClassFileLocation(std::string fileName)
    : mFileName(std::move(fileName))
  {
  }

  JvmExpected<std::unique_ptr<JClass>> resolve() override;

private:
  std::string mFileName;
};

class JarLocation : public ClassLocation
{
public:
  JarLocation(std::string archiveLocation, std::string fileName)
    : mArchiveLocation(std::move(archiveLocation)), mFileName(std::move(fileName))
  {
  }

  JvmExpected<std::unique_ptr<JClass>> resolve() override;

private:
  std::string mArchiveLocation;
  std::string mFileName;
};

class ClassLoader
{
public:
  virtual JvmExpected<std::unique_ptr<JClass>> loadClass(const types::JString& name) = 0;
  virtual ~ClassLoader() = default;

protected:
  JvmExpected<std::unique_ptr<JClass>> resolveClassLocation(const ClassLocation& location);
};

class BootstrapClassLoader
{
public:
  BootstrapClassLoader();

  JvmExpected<JClass*> loadClass(const types::JString& name);

private:
  std::string mBootstrapClassPath;
  std::unique_ptr<ClassLoader> mNextClassLoader;
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mClasses;
};

class BaseClassLoader : public ClassLoader
{
public:
  JvmExpected<std::unique_ptr<JClass>> loadClass(const types::JString& name) override;

private:
  std::optional<ClassLocation> findClassLocation(types::JStringRef name) const;
};

} // namespace geevm

#endif // GEEVM_VM_CLASSLOADER_H

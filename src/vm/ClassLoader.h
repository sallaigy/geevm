#ifndef GEEVM_VM_CLASSLOADER_H
#define GEEVM_VM_CLASSLOADER_H

#include "common/JvmError.h"
#include "common/JvmTypes.h"
#include "vm/Class.h"
#include "vm/ClassPath.h"

namespace geevm
{

class ZipArchive;

class ClassLoader
{
public:
  virtual JvmExpected<std::unique_ptr<InstanceClass>> loadClass(const types::JString& name) = 0;
  virtual ~ClassLoader() = default;

  ClassPath& classPath()
  {
    return mClassPath;
  }

protected:
  ClassPath mClassPath;
};

class BootstrapClassLoader
{
public:
  explicit BootstrapClassLoader(Vm& vm);

  ClassPath& classPath()
  {
    return mClassPath;
  }

  JvmExpected<JClass*> loadClass(const types::JString& name);
  JvmExpected<JClass*> loadUnpreparedClass(const types::JString& name);

  void registerClassLoader(std::unique_ptr<ClassLoader> classLoader);

private:
  JvmExpected<ArrayClass*> loadArrayClass(const types::JString& name);

private:
  Vm& mVm;
  ClassPath mClassPath;
  std::vector<std::unique_ptr<ClassLoader>> mClassLoaders;
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mClasses;
};

class BaseClassLoader : public ClassLoader
{
public:
  JvmExpected<std::unique_ptr<InstanceClass>> loadClass(const types::JString& name) override;
};

} // namespace geevm

#endif // GEEVM_VM_CLASSLOADER_H

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

protected:
  JvmExpected<std::unique_ptr<InstanceClass>> resolveClassLocation(const ClassLocation& location);
};

class BootstrapClassLoader
{
public:
  explicit BootstrapClassLoader(Vm& vm);

  JvmExpected<JClass*> loadClass(const types::JString& name);

private:
  JvmExpected<ArrayClass*> loadArrayClass(const types::JString& name);

private:
  Vm& mVm;
  std::unique_ptr<ZipArchive> mBootstrapArchive;
  std::unique_ptr<ClassLoader> mNextClassLoader;
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mClasses;
};

class BaseClassLoader : public ClassLoader
{
public:
  JvmExpected<std::unique_ptr<InstanceClass>> loadClass(const types::JString& name) override;

private:
  std::optional<ClassLocation> findClassLocation(types::JStringRef name) const;
};

} // namespace geevm

#endif // GEEVM_VM_CLASSLOADER_H

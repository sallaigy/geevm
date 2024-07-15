#ifndef GEEVM_VM_CLASSLOADER_H
#define GEEVM_VM_CLASSLOADER_H

#include "common/JvmTypes.h"
#include "common/JvmError.h"
#include "vm/Class.h"

namespace geevm
{

class ClassLocation
{
public:
  enum class Kind
  {
    File,
    Jar
  };

  ClassLocation(Kind kind, std::string  location)
    : mKind(kind), mLocation(std::move(location))
  {}

  bool isFile() const
  {
    return mKind == Kind::File;
  }

  const std::string& location() const
  {
    return mLocation;
  }

private:
  Kind mKind;
  std::string mLocation;
};

class ClassLoader
{
public:
  JvmExpected<JClass*> loadClass(const types::JString& name);

private:
  std::optional<ClassLocation> findClassLocation(types::JStringRef name) const;

private:
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mClasses;
};

}

#endif //GEEVM_VM_CLASSLOADER_H

#ifndef GEEVM_VM_METHOD_H
#define GEEVM_VM_METHOD_H

#include "class_file/ClassFile.h"

namespace geevm
{

struct MethodRef
{
  types::JStringRef className;
  types::JStringRef methodName;
  types::JStringRef methodDescriptor;
};

class JMethod
{
public:
  explicit JMethod(const MethodInfo& methodInfo) : mMethodInfo(methodInfo)
  {}

  const MethodInfo& getMethodInfo() const
  {
    return mMethodInfo;
  }

  const Code& getCode()
  {
    return mMethodInfo.code();
  }

private:
  const MethodInfo& mMethodInfo;
};

}

#endif //GEEVM_VM_METHOD_H

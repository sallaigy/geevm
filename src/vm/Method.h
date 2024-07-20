#ifndef GEEVM_VM_METHOD_H
#define GEEVM_VM_METHOD_H

#include <class_file/Descriptor.h>

#include "class_file/ClassFile.h"

namespace geevm
{

struct MethodRef
{
  types::JString className;
  types::JString methodName;
  types::JString methodDescriptor;
};

class JMethod
{
public:
  explicit JMethod(const MethodInfo& methodInfo, MethodDescriptor descriptor)
    : mMethodInfo(methodInfo), mDescriptor(descriptor)
  {
  }

  const MethodInfo& getMethodInfo() const
  {
    return mMethodInfo;
  }

  const Code& getCode()
  {
    return mMethodInfo.code();
  }

  MethodDescriptor getDescriptor() const
  {
    return mDescriptor;
  }

private:
  const MethodInfo& mMethodInfo;
  MethodDescriptor mDescriptor;
};

} // namespace geevm

#endif // GEEVM_VM_METHOD_H

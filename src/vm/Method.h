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
  explicit JMethod(const MethodInfo& methodInfo, types::JString name, types::JString rawDescriptor, MethodDescriptor descriptor)
    : mMethodInfo(methodInfo), mName(name), mRawDescriptor(rawDescriptor), mDescriptor(std::move(descriptor))
  {
  }

  MethodAccessFlags accessFlags() const
  {
    return mMethodInfo.accessFlags();
  }

  bool isStatic() const
  {
    return hasAccessFlag(mMethodInfo.accessFlags(), MethodAccessFlags::ACC_STATIC);
  }

  bool isNative() const
  {
    return hasAccessFlag(mMethodInfo.accessFlags(), MethodAccessFlags::ACC_NATIVE);
  }

  const MethodInfo& getMethodInfo() const
  {
    return mMethodInfo;
  }

  const Code& getCode()
  {
    return mMethodInfo.code();
  }

  const types::JString& name() const
  {
    return mName;
  }

  const MethodDescriptor& descriptor() const
  {
    return mDescriptor;
  }

  const types::JString& rawDescriptor() const
  {
    return mRawDescriptor;
  }

private:
  const MethodInfo& mMethodInfo;
  types::JString mName;
  types::JString mRawDescriptor;
  MethodDescriptor mDescriptor;
};

} // namespace geevm

#endif // GEEVM_VM_METHOD_H

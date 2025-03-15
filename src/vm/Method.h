#ifndef GEEVM_VM_METHOD_H
#define GEEVM_VM_METHOD_H

#include "class_file/ClassFile.h"
#include "class_file/Descriptor.h"
#include "vm/StackMap.h"

#include <utility>

namespace geevm
{
class InstanceClass;
class JClass;

class JMethod
{
public:
  JMethod(const MethodInfo& methodInfo, InstanceClass* klass, types::JString name, types::JString rawDescriptor, MethodDescriptor descriptor);

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

  bool isVoid() const
  {
    return mDescriptor.returnType().isVoid();
  }

  bool isAbstract() const
  {
    return hasAccessFlag(mMethodInfo.accessFlags(), MethodAccessFlags::ACC_ABSTRACT);
  }

  const MethodInfo& getMethodInfo() const
  {
    return mMethodInfo;
  }

  const Code& getCode() const
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

  InstanceClass* getClass() const
  {
    return mClass;
  }

private:
  const MethodInfo& mMethodInfo;
  InstanceClass* mClass;
  types::JString mName;
  types::JString mRawDescriptor;
  MethodDescriptor mDescriptor;
};

} // namespace geevm

#endif // GEEVM_VM_METHOD_H

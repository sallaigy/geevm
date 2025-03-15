#include "vm/Method.h"
#include "vm/Class.h"

using namespace geevm;

JMethod::JMethod(const MethodInfo& methodInfo, InstanceClass* klass, types::JString name, types::JString rawDescriptor, MethodDescriptor descriptor)
  : mMethodInfo(methodInfo), mClass(klass), mName(std::move(name)), mRawDescriptor(std::move(rawDescriptor)), mDescriptor(std::move(descriptor))
{
}

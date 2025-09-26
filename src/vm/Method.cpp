#include "vm/Method.h"
#include "vm/Class.h"

#include <common/Encoding.h>

using namespace geevm;

JMethod::JMethod(const MethodInfo& methodInfo, InstanceClass* klass, types::JString name, types::JString rawDescriptor, MethodDescriptor descriptor)
  : mMethodInfo(methodInfo), mClass(klass), mName(std::move(name)), mRawDescriptor(std::move(rawDescriptor)), mDescriptor(std::move(descriptor))
{
}

std::string JMethod::signatureString() const
{
  return std::format("{}#{}{}", utf16ToUtf8(mClass->className()), utf16ToUtf8(mName), utf16ToUtf8(mRawDescriptor));
}

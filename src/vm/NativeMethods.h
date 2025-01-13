#ifndef GEEVM_NATIVEMETHODS_H
#define GEEVM_NATIVEMETHODS_H

#include "vm/Frame.h"

#include <jni.h>

namespace geevm
{

class JavaThread;
class Vm;
class CallFrame;

class NativeMethod
{
  friend class NativeMethodRegistry;

  explicit NativeMethod(const JMethod* method, void* handle)
    : mMethod(method), mHandle(handle)
  {
  }

public:
  std::optional<Value> invoke(JavaThread& thread, const std::vector<Value>& args);

private:
  std::optional<Value> translateReturnValue(jvalue returnValue) const;

  const JMethod* mMethod;
  void* mHandle;
};

class NativeMethodRegistry
{
public:
  std::optional<NativeMethod> getNativeMethod(const JMethod* method);
};

} // namespace geevm

#endif // GEEVM_NATIVEMETHODS_H

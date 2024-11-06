#ifndef GEEVM_THREAD_H
#define GEEVM_THREAD_H

#include "vm/Frame.h"

#include <common/JvmError.h>
#include <list>

namespace geevm
{
class JavaHeap;
}
namespace geevm
{

class Vm;

class JavaThread
{
public:
  // Constructor
  //==------------------------------------------------------------------------==
  explicit JavaThread(Vm& vm)
    : mVm(vm)
  {
  }

  void initialize(const types::JString& name, Instance* threadGroup);

  // Thread state and queries
  //==------------------------------------------------------------------------==

  Instance* instance()
  {
    return mThreadInstance;
  }

  // Virtual machine and heap access
  //==------------------------------------------------------------------------==
  JavaHeap& heap();
  JvmExpected<JClass*> resolveClass(const types::JString& name);

  // Call stack
  //==------------------------------------------------------------------------==
  CallFrame& currentFrame()
  {
    return mCallStack.back();
  }

  void returnToCaller(Value returnValue);

  std::optional<Value> executeCall(JMethod* method, const std::vector<Value>& args);
  std::optional<Value> executeNative(JMethod* method, CallFrame& frame, const std::vector<Value>& args);

  std::optional<Value> invoke(JMethod* method);

  // Exception handling
  //==------------------------------------------------------------------------==

  void throwException(Instance* exceptionInstance);
  void throwException(const types::JString& name, const types::JString& message = u"");

  void clearException();

  Instance* currentException() const
  {
    return mCurrentException;
  }

private:
  Vm& mVm;
  Instance* mThreadInstance;
  std::list<CallFrame> mCallStack;
  Instance* mCurrentException = nullptr;
};

} // namespace geevm

#endif // GEEVM_THREAD_H

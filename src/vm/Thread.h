#ifndef GEEVM_THREAD_H
#define GEEVM_THREAD_H

#include "common/JvmError.h"
#include "vm/Frame.h"
#include "vm/GarbageCollector.h"

#include <list>
#include <thread>

namespace geevm
{

class JavaHeap;
class Vm;

class JavaThread
{
public:
  // Constructor
  //==------------------------------------------------------------------------==
  explicit JavaThread(Vm& vm);

  void initialize(const types::JString& name, Instance* threadGroup);

  // Thread state
  //==------------------------------------------------------------------------==

  void start(JMethod* method, std::vector<Value> arguments);

  void run();

  // Getters
  //==------------------------------------------------------------------------==
  Vm& vm()
  {
    return mVm;
  }

  std::jthread& nativeThread()
  {
    return mNativeThread;
  }

  GcRootRef<Instance> instance()
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

  std::list<CallFrame>& callStack()
  {
    return mCallStack;
  }

  void returnToCaller(Value returnValue);

  std::optional<Value> executeCall(JMethod* method, const std::vector<Value>& args);
  std::optional<Value> executeNative(JMethod* method, CallFrame& frame, const std::vector<Value>& args);

  std::optional<Value> invoke(JMethod* method);

  // Exception handling
  //==------------------------------------------------------------------------==

  void throwException(Instance* exceptionInstance);
  void throwException(const types::JString& name, const types::JString& message = u"");
  Instance* createStackTrace();

  void clearException();

  GcRootRef<Instance> currentException() const
  {
    return mCurrentException;
  }

private:
  Vm& mVm;
  // Method to run and arguments
  JMethod* mMethod = nullptr;
  std::vector<Value> mArguments;
  GcRootRef<Instance> mThreadInstance;
  std::list<CallFrame> mCallStack;
  GcRootRef<Instance> mCurrentException;
  std::jthread mNativeThread;
};

} // namespace geevm

#endif // GEEVM_THREAD_H

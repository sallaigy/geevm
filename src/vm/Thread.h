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

  GcRootRef<> instance()
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

  std::optional<Value> invoke(JMethod* method);
  std::optional<Value> invokeWithArgs(JMethod* method, std::vector<Value> arguments);

  // Exception handling
  //==------------------------------------------------------------------------==

  void throwException(Instance* exceptionInstance);
  void throwException(const types::JString& name, const types::JString& message = u"");
  Instance* createStackTrace();

  void clearException();

  GcRootRef<> currentException() const
  {
    return mCurrentException;
  }

  // JNI references
  //==------------------------------------------------------------------------==

  /// Marks the given object as JNI reference in the current native frame.
  /// JNI references are GC-safe as long as the stack frame that produced them is alive.
  /// Note that this method call is valid only in native frames.
  GcRootRef<Instance> addJniHandle(Instance* instance);

private:
  std::optional<Value> executeCall(JMethod* method, CallFrame* current, CallFrame& newFrame);
  std::optional<Value> executeNative(JMethod* method, CallFrame& frame, std::vector<Value> arguments);
  void handleCalleeException(CallFrame* callerFrame);

  void run();
  void prepareNativeFrame();
  void releaseNativeFrame();

private:
  Vm& mVm;
  // Method to run and arguments
  JMethod* mMethod = nullptr;
  std::vector<Value> mArguments;
  GcRootRef<Instance> mThreadInstance;
  std::list<CallFrame> mCallStack;

  // Exceptions
  GcRootRef<Instance> mCurrentException;
  // True if the thread is executing an uncaught exception handler
  bool mHasUncaughtException = false;

  // List of JNI references
  std::vector<std::vector<GcRootRef<>>> mJniHandles;

  std::jthread mNativeThread;
};

} // namespace geevm

#endif // GEEVM_THREAD_H

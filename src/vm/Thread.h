#ifndef GEEVM_THREAD_H
#define GEEVM_THREAD_H

#include "common/JvmError.h"
#include "vm/GarbageCollector.h"
#include "vm/Value.h"

#include <generator>
#include <thread>

namespace geevm
{

class JavaHeap;
class Vm;
class CallFrame;
class JMethod;

class JavaThread
{
public:
  // Constructors and destructor
  //==------------------------------------------------------------------------==
  explicit JavaThread(Vm& vm);
  JavaThread(const JavaThread&) = delete;

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
    assert(mCurrentFrame != nullptr);
    return *mCurrentFrame;
  }

  std::generator<CallFrame&> callStack();

  bool isCallStackEmpty()
  {
    return mCurrentFrame == nullptr;
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
  /// Executes the topmost frame of the call stack
  std::optional<Value> executeTopFrame();
  std::optional<Value> executeNative(JMethod* method, CallFrame& frame, std::vector<Value> arguments);
  void handleCalleeException(CallFrame* callerFrame);

  void run();
  void prepareNativeFrame();
  void releaseNativeFrame();

  CallFrame* newFrame(JMethod* method);
  void popFrame();
  void* allocateCallFrameSpace(size_t size);

private:
  Vm& mVm;
  // Method to run and arguments
  JMethod* mMethod = nullptr;
  std::vector<Value> mArguments;
  GcRootRef<Instance> mThreadInstance;

  std::unique_ptr<char[]> mCallStackSpace = nullptr;
  char* mCallStackTop = nullptr;

  CallFrame* mCurrentFrame = nullptr;

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

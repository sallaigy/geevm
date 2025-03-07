#include "vm/Thread.h"
#include "common/Memory.h"
#include "vm/Instance.h"
#include "vm/Interpreter.h"
#include "vm/Vm.h"
#include "vm/VmUtils.h"

#include <algorithm>
#include <iostream>
#include <ranges>

using namespace geevm;

JavaThread::JavaThread(Vm& vm)
  : mVm(vm), mCurrentException(nullptr), mThreadInstance(nullptr)
{
  mCallStackSpace = std::unique_ptr<char[]>(new char[vm.settings().maxStackSize]);
  mCallStackTop = mCallStackSpace.get();
}

void JavaThread::initialize(const types::JString& name, Instance* threadGroup)
{
  auto klass = mVm.resolveClass(u"java/lang/Thread");
  assert(klass.has_value());

  mThreadInstance = heap().gc().pin(heap().allocate<ObjectInstance>((*klass)->asInstanceClass())).release();

  auto nameInstance = heap().intern(name);
  mThreadInstance->setFieldValue<Instance*>(u"name", u"Ljava/lang/String;", nameInstance.get());
  mThreadInstance->setFieldValue<Instance*>(u"group", u"Ljava/lang/ThreadGroup;", threadGroup);
  mThreadInstance->setFieldValue<Instance*>(u"uncaughtExceptionHandler", u"Ljava/lang/Thread$UncaughtExceptionHandler;", threadGroup);
  mThreadInstance->setFieldValue<int32_t>(u"priority", u"I", 10);
}

void JavaThread::start(JMethod* method, std::vector<Value> arguments)
{
  mMethod = method;
  mArguments = std::move(arguments);
  mNativeThread = std::jthread([this]() {
    this->run();
  });
  mThreadInstance->setFieldValue<int64_t>(u"eetop", u"J", (long)mNativeThread.native_handle());
}

void JavaThread::run()
{
  this->invokeWithArgs(mMethod, mArguments);
}

JavaHeap& JavaThread::heap()
{
  return mVm.heap();
}

JvmExpected<JClass*> JavaThread::resolveClass(const types::JString& name)
{
  return mVm.resolveClass(name);
}

std::generator<CallFrame&> JavaThread::callStack()
{
  CallFrame* current = mCurrentFrame;
  while (current != nullptr) {
    co_yield *current;
    current = current->previous();
  }
}

std::optional<Value> JavaThread::invoke(JMethod* method)
{
  CallFrame* current = mCurrentFrame;
  CallFrame* newFrame = this->newFrame(method);

  std::optional<Value> returnValue;

  if (!method->isNative()) {
    size_t numSlots = method->descriptor().numParameterSlots();
    if (!method->isStatic()) {
      numSlots += 1;
    }
    current->prepareCall(*newFrame, numSlots);

    returnValue = this->executeCall(method, current, *newFrame);
  } else {
    std::vector<Value> args;
    for (const auto& param : std::ranges::reverse_view(method->descriptor().parameters())) {
      if (param.isCategoryTwo()) {
        current->popGenericOperand();
      }
      args.push_back(current->popGenericOperand());
    }

    if (!method->isStatic()) {
      args.push_back(current->popGenericOperand());
    }

    assert(method->isStatic() ? method->descriptor().parameters().size() == args.size() : method->descriptor().parameters().size() == args.size() - 1);
    std::ranges::reverse(args);

    returnValue = this->executeNative(method, *newFrame, args);
  }

  this->popFrame();
  this->handleCalleeException(current);

  return returnValue;
}

std::optional<Value> JavaThread::invokeWithArgs(JMethod* method, std::vector<Value> arguments)
{
  CallFrame* current = mCurrentFrame;
  CallFrame* newFrame = this->newFrame(method);

  std::optional<Value> returnValue;
  if (method->isNative()) {
    returnValue = this->executeNative(method, *newFrame, arguments);
  } else {
    uint16_t argIndex = 0;
    uint16_t instanceCallOffset = 0;
    if (!method->isStatic()) {
      newFrame->storeValue<Instance*>(0, arguments[0].get<Instance*>());
      argIndex += 1;
      instanceCallOffset = 1;
    }

    for (size_t i = 0; i < method->descriptor().parameters().size(); ++i) {
      auto& param = method->descriptor().parameters()[i];
      newFrame->storeGenericValue(argIndex, arguments[i + instanceCallOffset].toRaw().first, arguments[i].toRaw().second);
      argIndex += 1;
      if (param.isCategoryTwo()) {
        argIndex += 1;
      }
    }

    returnValue = this->executeCall(method, current, *newFrame);
  }

  this->popFrame();
  this->handleCalleeException(current);

  return returnValue;
}

std::optional<Value> JavaThread::executeCall(JMethod* method, CallFrame* current, CallFrame& newFrame)
{
  std::optional<Value> returnValue;
  auto interpreter = createDefaultInterpreter(*this);

  return interpreter->execute(method->getCode(), 0);
}

void JavaThread::handleCalleeException(CallFrame* callerFrame)
{
  if (mCurrentException != nullptr) {
    if (callerFrame != nullptr) {
      auto callerMethod = callerFrame->currentMethod();
      if (callerMethod->isNative() || callerMethod->getCode().exceptionTable().empty()) {
        // The caller frame won't be able to handle the exception and pushing the exception to the
        // operand stack is not safe. Let the caller of the caller handle it.
        return;
      }
      callerFrame->clearOperandStack();
      callerFrame->pushOperand<Instance*>(mCurrentException.get());
    } else {
      auto handler = mThreadInstance->getFieldValue<Instance*>(u"uncaughtExceptionHandler", u"Ljava/lang/Thread$UncaughtExceptionHandler;");
      auto handlerMethod = handler->getClass()->getVirtualMethod(u"uncaughtException", u"(Ljava/lang/Thread;Ljava/lang/Throwable;)V");
      assert(handlerMethod.has_value());

      Instance* currentException = mCurrentException.get();
      this->clearException();

      mHasUncaughtException = true;
      this->invokeWithArgs(*handlerMethod, {Value::from(handler), Value::from(mThreadInstance.get()), Value::from(currentException)});

      // TODO: We should exit with exit code 1, but some of our current tests would break
      std::exit(0);
    }
  }
}

std::optional<Value> JavaThread::executeNative(JMethod* method, CallFrame& frame, std::vector<Value> arguments)
{
  auto nativeHandle = mVm.nativeMethods().getNativeMethod(method);
  if (nativeHandle) {
    this->prepareNativeFrame();
    std::optional<Value> returnValue = nativeHandle->invoke(*this, arguments);
    this->releaseNativeFrame();
    return returnValue;
  }

  types::JString name;
  name += method->getClass()->javaClassName();
  name += u".";
  name += method->name();

  this->throwException(u"java/lang/UnsatisfiedLinkError", method->descriptor().formatAsJavaSignature(name));
  return std::nullopt;
}

void JavaThread::throwException(Instance* exceptionInstance)
{
  if (mHasUncaughtException) {
    // Exception while executing an uncaught exception handler - abort immediately
    geevm_panic("Exception thrown while executing uncaught exception handler");
  }

  assert(mCurrentException == nullptr && "There is already an exception instance");
  mCurrentException = mVm.heap().gc().pin(exceptionInstance).release();
  if (!currentFrame().currentMethod()->isNative()) {
    currentFrame().clearOperandStack();
    currentFrame().pushOperand(exceptionInstance);
  }
}

void JavaThread::throwException(const types::JString& name, const types::JString& message)
{
  auto klass = mVm.resolveClass(name);
  if (!klass) {
    // TODO: handle failure to resolve exception class
    geevm_panic("failure to resolve exception class");
  }

  GcRootRef<> exceptionInstance = heap().gc().pin(heap().allocate<ObjectInstance>((*klass)->asInstanceClass())).release();
  GcRootRef<> messageInstance = heap().intern(message);
  exceptionInstance->setFieldValue(u"detailMessage", u"Ljava/lang/String;", messageInstance.get());

  auto stackTrace = createStackTrace();
  exceptionInstance->setFieldValue(u"stackTrace", u"[Ljava/lang/StackTraceElement;", stackTrace);

  this->throwException(exceptionInstance.get());
}

void JavaThread::clearException()
{
  assert(mCurrentException != nullptr && "There should be an exception instance");
  heap().gc().release(mCurrentException);
  mCurrentException = nullptr;
}

static int32_t getFrameLineNumber(const CallFrame& callFrame)
{
  if (!callFrame.currentMethod()->isNative()) {
    int32_t lineNumber = -1;
    size_t pc = callFrame.programCounter();
    const Code& code = callFrame.currentMethod()->getCode();
    auto& lineNumbers = code.lineNumberTable();
    for (size_t i = 0; i < lineNumbers.size(); ++i) {
      if (i == lineNumbers.size() - 1) {
        // This is the last entry in the table
        if (lineNumbers[i].startPc < pc) {
          lineNumber = lineNumbers[i].lineNumber;
        }
      } else if (lineNumbers[i].startPc <= pc && pc <= lineNumbers[i + 1].startPc) {
        lineNumber = lineNumbers[i].lineNumber;
      }
    }

    if (lineNumber != -1) {
      return lineNumber;
    }
  } else {
    // -2 is the magic line number in the JDK for native methods
    return -2;
  }

  // We signify unknown line numbers as -1
  return -1;
}

Instance* JavaThread::createStackTrace()
{
  auto stackTraceElementCls = resolveClass(u"java/lang/StackTraceElement");
  assert(stackTraceElementCls);
  auto stackTraceArrayCls = resolveClass(u"[Ljava/lang/StackTraceElement;");
  assert(stackTraceArrayCls.has_value());

  auto throwable = resolveClass(u"java/lang/Throwable");

  std::vector<ScopedGcRootRef<>> stackTrace;

  bool include = false;

  std::vector<CallFrame*> callStackVector;
  for (CallFrame& frame : this->callStack()) {
    callStackVector.push_back(&frame);
  }

  for (const CallFrame* framePtr : callStackVector) {
    const CallFrame& callFrame = *framePtr;
    if (!callFrame.currentClass()->isInstanceOf(*throwable)) {
      include = true;
    }

    if (include) {
      auto stackTraceElement = heap().gc().pin(heap().allocate<ObjectInstance>((*stackTraceElementCls)->asInstanceClass()));
      auto declaringClass = heap().intern(callFrame.currentClass()->javaClassName());
      auto declaringClassObject = callFrame.currentClass()->classInstance();
      auto methodName = heap().intern(callFrame.currentMethod()->name());

      GcRootRef<> sourceFile = nullptr;
      if (auto sourceFileStr = callFrame.currentClass()->sourceFile(); sourceFileStr) {
        sourceFile = heap().intern(*sourceFileStr);
      }

      stackTraceElement->setFieldValue<Instance*>(u"declaringClass", u"Ljava/lang/String;", declaringClass.get());
      stackTraceElement->setFieldValue<Instance*>(u"declaringClassObject", u"Ljava/lang/Class;", declaringClassObject.get());
      stackTraceElement->setFieldValue<Instance*>(u"methodName", u"Ljava/lang/String;", methodName.get());
      stackTraceElement->setFieldValue<Instance*>(u"fileName", u"Ljava/lang/String;", sourceFile.get());

      int32_t lineNumber = getFrameLineNumber(callFrame);
      if (lineNumber != -1) {
        stackTraceElement->setFieldValue<int32_t>(u"lineNumber", u"I", getFrameLineNumber(callFrame));
      }

      stackTrace.emplace_back(std::move(stackTraceElement));
    }
  }

  JavaArray<Instance*>* array = heap().allocateArray<Instance*>((*stackTraceArrayCls)->asArrayClass(), stackTrace.size());
  for (int32_t i = 0; i < stackTrace.size(); i++) {
    array->setArrayElement(i, stackTrace[i].get());
  }

  return array;
}

GcRootRef<> JavaThread::addJniHandle(Instance* instance)
{
  assert(currentFrame().currentMethod()->isNative());
  GcRootRef<>& handle = mJniHandles.back().emplace_back(this->heap().gc().pin(instance).release());

  return handle;
}

void JavaThread::prepareNativeFrame()
{
  assert(currentFrame().currentMethod()->isNative());
  mJniHandles.emplace_back();
}

void JavaThread::releaseNativeFrame()
{
  assert(currentFrame().currentMethod()->isNative());
  auto& handles = mJniHandles.back();
  for (GcRootRef<>& handle : handles) {
    heap().gc().release(handle);
  }

  mJniHandles.pop_back();
}

CallFrame* JavaThread::newFrame(JMethod* method)
{
  size_t allocationSize = sizeof(CallFrame);

  CallFrame* callFrame = nullptr;
  if (method->isNative()) {
    void* mem = this->allocateCallFrameSpace(allocationSize);
    callFrame = new (mem) CallFrame(method, mCurrentFrame);
  } else {
    size_t localsOffset = alignTo(allocationSize, alignof(uint64_t));
    allocationSize = localsOffset;
    allocationSize += method->getCode().maxLocals() * sizeof(uint64_t);

    size_t localRefsOffset = alignTo(allocationSize, alignof(bool));
    allocationSize = localRefsOffset;
    allocationSize += method->getCode().maxLocals() * sizeof(bool);

    size_t stackOffset = alignTo(allocationSize, alignof(uint64_t));
    allocationSize = stackOffset;
    allocationSize += method->getCode().maxStack() * sizeof(uint64_t);

    size_t stackRefsOffset = alignTo(allocationSize, alignof(uint64_t));
    allocationSize = stackRefsOffset;
    allocationSize += method->getCode().maxStack() * sizeof(bool);

    allocationSize = alignTo(allocationSize, alignof(CallFrame));

    char* mem = static_cast<char*>(this->allocateCallFrameSpace(allocationSize));

    uint64_t* localVariables = reinterpret_cast<uint64_t*>(mem + localsOffset);
    bool* localRefs = reinterpret_cast<bool*>(mem + localRefsOffset);
    uint64_t* stack = reinterpret_cast<uint64_t*>(mem + stackOffset);
    bool* stackRefs = reinterpret_cast<bool*>(mem + stackRefsOffset);

    callFrame = new (mem) CallFrame(method, mCurrentFrame, localVariables, localRefs, stack, stackRefs);
  }

  mCurrentFrame = callFrame;

  return callFrame;
}

void* JavaThread::allocateCallFrameSpace(size_t allocationSize)
{
  if (mCallStackTop + allocationSize >= mCallStackSpace.get() + mVm.settings().maxStackSize) {
    // TODO: Throw StackOverflowError
    geevm_panic("Out of stack memory");
  }

  void* ptr = mCallStackTop;
  mCallStackTop += allocationSize;

  return ptr;
}

void JavaThread::popFrame()
{
  assert(mCurrentFrame != nullptr);
  mCallStackTop = reinterpret_cast<char*>(mCurrentFrame);
  mCurrentFrame = mCurrentFrame->previous();
}

#include "vm/Thread.h"
#include "vm/Instance.h"
#include "vm/Interpreter.h"
#include "vm/Vm.h"
#include "vm/VmUtils.h"

#include <algorithm>
#include <iostream>

using namespace geevm;

JavaThread::JavaThread(Vm& vm)
  : mVm(vm), mCurrentException(nullptr), mThreadInstance(nullptr)
{
}

void JavaThread::initialize(const types::JString& name, Instance* threadGroup)
{
  auto klass = mVm.resolveClass(u"java/lang/Thread");
  assert(klass.has_value());

  mThreadInstance = heap().gc().pin(heap().allocate((*klass)->asInstanceClass())).release();

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
  this->executeCall(mMethod, mArguments);
}

JavaHeap& JavaThread::heap()
{
  return mVm.heap();
}

JvmExpected<JClass*> JavaThread::resolveClass(const types::JString& name)
{
  return mVm.resolveClass(name);
}

std::optional<Value> JavaThread::invoke(JMethod* method)
{
  CallFrame& current = currentFrame();
  std::vector<Value> args;

  for (int i = 0; i < method->descriptor().parameters().size(); i++) {
    Value value = current.popGenericOperand();
    args.push_back(value);
    if (value.isCategoryTwo()) {
      args.push_back(value);
    }
  }

  if (!method->isStatic()) {
    args.push_back(Value::from(current.popOperand<Instance*>()));
  }

  std::ranges::reverse(args);

  return this->executeCall(method, args);
}

std::optional<Value> JavaThread::executeCall(JMethod* method, const std::vector<Value>& args)
{
  CallFrame* current = mCallStack.empty() ? nullptr : &mCallStack.back();
  CallFrame& frame = mCallStack.emplace_back(method, current);

  std::optional<Value> returnValue;
  if (method->isNative()) {
    this->prepareNativeFrame();
    returnValue = this->executeNative(method, frame, args);
    this->releaseNativeFrame();
  } else {
    for (int i = 0; i < args.size(); ++i) {
      frame.storeGenericValue(i, args[i]);
    }

    auto interpreter = createDefaultInterpreter(*this);
    returnValue = interpreter->execute(method->getCode(), 0);
  }

  mCallStack.pop_back();
  if (mCurrentException != nullptr) {
    if (current != nullptr) {
      current->clearOperandStack();
      current->pushOperand<Instance*>(mCurrentException.get());
    } else {
      auto handler = mThreadInstance->getFieldValue<Instance*>(u"uncaughtExceptionHandler", u"Ljava/lang/Thread$UncaughtExceptionHandler;");
      auto handlerMethod = handler->getClass()->getVirtualMethod(u"uncaughtException", u"(Ljava/lang/Thread;Ljava/lang/Throwable;)V");
      assert(handlerMethod.has_value());

      Instance* currentException = mCurrentException.get();
      this->clearException();
      this->executeCall((*handlerMethod), {Value::from(handler), Value::from(mThreadInstance.get()), Value::from(currentException)});
      // TODO: We should exit with exit code 1, but some of our current tests would break
      std::exit(0);
    }
  }

  return returnValue;
}

std::optional<Value> JavaThread::executeNative(JMethod* method, CallFrame& frame, const std::vector<Value>& args)
{
  auto nativeHandle = mVm.nativeMethods().getNativeMethod(method);
  if (nativeHandle) {
    return nativeHandle->invoke(*this, args);
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
  assert(mCurrentException == nullptr && "There is already an exception instance");
  mCurrentException = mVm.heap().gc().pin(exceptionInstance).release();
  currentFrame().clearOperandStack();
  currentFrame().pushOperand(exceptionInstance);
}

void JavaThread::throwException(const types::JString& name, const types::JString& message)
{
  auto klass = mVm.resolveClass(name);
  if (!klass) {
    // TODO: handle failure to resolve exception class
    geevm_panic("failure to resolve exception class");
  }

  GcRootRef<Instance> exceptionInstance = heap().gc().pin(heap().allocate((*klass)->asInstanceClass())).release();
  GcRootRef<Instance> messageInstance = heap().intern(message);
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
  for (auto it = callStack().rbegin(); it != callStack().rend(); ++it) {
    const CallFrame& callFrame = *it;
    if (!callFrame.currentClass()->isInstanceOf(*throwable)) {
      include = true;
    }

    if (include) {
      auto stackTraceElement = heap().gc().pin(heap().allocate((*stackTraceElementCls)->asInstanceClass()));
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

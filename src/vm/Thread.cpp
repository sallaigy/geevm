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

  mThreadInstance = heap().gc().pin(heap().allocate((*klass)->asInstanceClass()));

  auto nameInstance = heap().intern(name);
  mThreadInstance->setFieldValue<Instance*>(u"name", u"Ljava/lang/String;", nameInstance);
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

Instance* JavaThread::newInstance(const types::JString& className)
{
  auto klass = mVm.resolveClass(className);
  if (!klass) {
    // TODO
    assert(false && "TODO");
  }

  return heap().allocate((*klass)->asInstanceClass());
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
    returnValue = this->executeNative(method, frame, args);
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
      Instance* handler = mThreadInstance->getFieldValue<Instance*>(u"uncaughtExceptionHandler", u"Ljava/lang/Thread$UncaughtExceptionHandler;");
      auto handlerMethod = handler->getClass()->getVirtualMethod(u"uncaughtException", u"(Ljava/lang/Thread;Ljava/lang/Throwable;)V");
      assert(handlerMethod.has_value());

      // FIXME: We should execute the handler, but we do not support `System.err` / `System.out` yet.
      // this->executeCall((*handlerMethod), {Value::Reference(handler), Value::Reference(mThreadInstance), Value::Reference(mCurrentException)});
      auto exceptionMessage = mCurrentException->getFieldValue<Instance*>(u"detailMessage", u"Ljava/lang/String;");
      assert(exceptionMessage->getClass()->className() == u"java/lang/String");

      types::JString message = u"Exception ";

      auto exceptionClsName = mCurrentException->getClass()->javaClassName();

      message += exceptionClsName;
      message += u": '";
      message += utils::getStringValue(exceptionMessage);
      message += u"'";

      // Instance* stackTrace = mCurrentException->getFieldValue(u"stackTrace", u"[Ljava/lang/StackTraceElement;").asReference();
      auto getStackTrace = mCurrentException->getClass()->getVirtualMethod(u"getOurStackTrace", u"()[Ljava/lang/StackTraceElement;");

      GcRootRef<Instance> exceptionInstance = mCurrentException;
      mCurrentException.reset();
      Instance* stackTrace = this->executeCall((*getStackTrace), {Value::from<Instance*>(exceptionInstance.get())})->get<Instance*>();

      if (stackTrace != nullptr) {
        message += u"\n";

        // We'll have to be careful while iterating here. The array and the stack trace elements may get relocated
        // by the garbage collector during iteration.
        GcRootRef<JavaArray<Instance*>> stackTraceArray = heap().gc().pin(stackTrace->asArray<Instance*>());
        for (int32_t i = 0; i < stackTraceArray->length(); i++) {
          Instance* elem = stackTraceArray->getArrayElement(i).value();

          auto toStringMethod = elem->getClass()->getVirtualMethod(u"toString", u"()Ljava/lang/String;");
          auto ret = this->executeCall(*toStringMethod, {Value::from<Instance*>(elem)});
          assert(ret.has_value());

          message += u"  at ";
          message += utils::getStringValue(ret->get<Instance*>());
          message += u"\n";
        }

        heap().gc().release(stackTraceArray);
      }

      std::cerr << types::convertJString(message) << std::endl;
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
  mCurrentException = mVm.heap().gc().pin(exceptionInstance);
  currentFrame().clearOperandStack();
  currentFrame().pushOperand(exceptionInstance);
}

void JavaThread::throwException(const types::JString& name, const types::JString& message)
{
  auto klass = mVm.resolveClass(name);
  if (!klass) {
    assert(false && "TODO: failure to resolve exception class");
  }

  GcRootRef<Instance> exceptionInstance = heap().gc().pin(heap().allocate((*klass)->asInstanceClass()));
  Instance* messageInstance = heap().intern(message);
  exceptionInstance->setFieldValue(u"detailMessage", u"Ljava/lang/String;", messageInstance);

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

Instance* JavaThread::createStackTrace()
{
  auto stackTraceElementCls = resolveClass(u"java/lang/StackTraceElement");
  assert(stackTraceElementCls);
  auto stackTraceArrayCls = resolveClass(u"[Ljava/lang/StackTraceElement;");
  assert(stackTraceArrayCls.has_value());

  auto throwable = resolveClass(u"java/lang/Throwable");

  std::vector<GcRootRef<>> stackTrace;

  bool include = false;
  for (auto it = callStack().rbegin(); it != callStack().rend(); ++it) {
    const CallFrame& callFrame = *it;
    if (!callFrame.currentClass()->isInstanceOf(*throwable)) {
      include = true;
    }

    if (include) {
      auto stackTraceElement = heap().gc().pin(heap().allocate((*stackTraceElementCls)->asInstanceClass()));
      stackTraceElement->setFieldValue<Instance*>(u"declaringClass", u"Ljava/lang/String;", heap().intern(callFrame.currentClass()->javaClassName()));
      stackTraceElement->setFieldValue<Instance*>(u"declaringClassObject", u"Ljava/lang/Class;", callFrame.currentClass()->classInstance().get());
      stackTraceElement->setFieldValue<Instance*>(u"methodName", u"Ljava/lang/String;", heap().intern(callFrame.currentMethod()->name()));
      stackTrace.push_back(stackTraceElement);
    }
  }

  JavaArray<Instance*>* array = heap().allocateArray<Instance*>((*stackTraceArrayCls)->asArrayClass(), stackTrace.size());
  for (int32_t i = 0; i < stackTrace.size(); i++) {
    array->setArrayElement(i, stackTrace[i].get());
  }

  return array;
}

#include "vm/Thread.h"
#include "vm/Interpreter.h"
#include "vm/Vm.h"
#include "vm/VmUtils.h"

#include <algorithm>
#include <iostream>

using namespace geevm;

JavaThread::JavaThread(Vm& vm)
  : mVm(vm)
{
}

void JavaThread::initialize(const types::JString& name, Instance* threadGroup)
{
  auto klass = mVm.resolveClass(u"java/lang/Thread");
  assert(klass.has_value());

  mThreadInstance = heap().allocate((*klass)->asInstanceClass());

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
  mNativeThread = std::jthread([this]() { this->run(); });
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
      current->pushOperand<Instance*>(mCurrentException);
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

      Instance* exceptionInstance = mCurrentException;
      mCurrentException = nullptr;
      Instance* stackTrace = this->executeCall((*getStackTrace), {Value::from<Instance*>(exceptionInstance)})->get<Instance*>();

      if (stackTrace != nullptr) {
        message += u"\n";

        auto& arrayContents = stackTrace->asArrayInstance()->contents();
        for (Value elem : arrayContents) {
          auto toStringMethod = elem.get<Instance*>()->getClass()->getVirtualMethod(u"toString", u"()Ljava/lang/String;");
          auto ret = this->executeCall(*toStringMethod, {elem});
          assert(ret.has_value());

          message += u"  at ";
          message += utils::getStringValue(ret->get<Instance*>());
          message += u"\n";
        }
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
  auto handle = mVm.nativeMethods().get(method);
  if (!handle) {
    types::JString name;
    name += method->getClass()->javaClassName();
    name += u".";
    name += method->name();

    this->throwException(u"java/lang/UnsatisfiedLinkError", method->descriptor().formatAsJavaSignature(name));
    return std::nullopt;
  }

  return (*handle)(*this, frame, args);
}

void JavaThread::throwException(Instance* exceptionInstance)
{
  assert(mCurrentException == nullptr && "There is already an exception instance");
  mCurrentException = exceptionInstance;
  currentFrame().clearOperandStack();
  currentFrame().pushOperand(exceptionInstance);
}

void JavaThread::throwException(const types::JString& name, const types::JString& message)
{
  auto klass = mVm.resolveClass(name);
  if (!klass) {
    assert(false && "TODO: failure to resolve exception class");
  }

  Instance* exceptionInstance = heap().allocate((*klass)->asInstanceClass());
  Instance* messageInstance = heap().intern(message);
  exceptionInstance->setFieldValue(u"detailMessage", u"Ljava/lang/String;", messageInstance);
  exceptionInstance->setFieldValue(u"stackTrace", u"[Ljava/lang/StackTraceElement;", createStackTrace());

  this->throwException(exceptionInstance);
}

void JavaThread::clearException()
{
  assert(mCurrentException != nullptr && "There should be an exception instance");
  mCurrentException = nullptr;
}

Instance* JavaThread::createStackTrace()
{
  auto stackTraceElementCls = resolveClass(u"java/lang/StackTraceElement");
  assert(stackTraceElementCls);
  auto stackTraceArrayCls = resolveClass(u"[Ljava/lang/StackTraceElement;");
  assert(stackTraceArrayCls.has_value());

  auto throwable = resolveClass(u"java/lang/Throwable");

  std::vector<Instance*> stackTrace;

  bool include = false;
  for (auto it = callStack().rbegin(); it != callStack().rend(); ++it) {
    const CallFrame& callFrame = *it;
    if (!callFrame.currentClass()->isInstanceOf(*throwable)) {
      include = true;
    }

    if (include) {
      auto stackTraceElement = heap().allocate((*stackTraceElementCls)->asInstanceClass());
      stackTraceElement->setFieldValue(u"declaringClass", u"Ljava/lang/String;", heap().intern(callFrame.currentClass()->javaClassName()));
      stackTraceElement->setFieldValue(u"methodName", u"Ljava/lang/String;", heap().intern(callFrame.currentMethod()->name()));
      stackTrace.push_back(stackTraceElement);
    }
  }

  ArrayInstance* array = heap().allocateArray((*stackTraceArrayCls)->asArrayClass(), stackTrace.size());
  for (int32_t i = 0; i < stackTrace.size(); i++) {
    array->setArrayElement(i, stackTrace[i]);
  }

  return array;
}

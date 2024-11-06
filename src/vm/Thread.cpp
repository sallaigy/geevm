#include "vm/Thread.h"

#include "Interpreter.h"
#include "Vm.h"

#include <algorithm>
#include <iostream>

using namespace geevm;

void JavaThread::initialize(const types::JString& name, Instance* threadGroup)
{
  auto klass = mVm.resolveClass(u"java/lang/Thread");
  assert(klass.has_value());

  mThreadInstance = heap().allocate((*klass)->asInstanceClass());

  auto nameInstance = heap().intern(name);
  mThreadInstance->setFieldValue(u"name", Value::Reference(nameInstance));

  auto address = std::bit_cast<int64_t>(this);
  mThreadInstance->setFieldValue(u"eetop", Value::Long(address));

  mThreadInstance->setFieldValue(u"group", Value::Reference(threadGroup));
  mThreadInstance->setFieldValue(u"uncaughtExceptionHandler", Value::Reference(threadGroup));
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
    Value value = current.popOperand();
    args.push_back(value);
    if (value.isCategoryTwo()) {
      args.push_back(value);
    }
  }

  if (!method->isStatic()) {
    args.push_back(current.popOperand());
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
      frame.storeValue(i, args[i]);
    }

    auto interpreter = createDefaultInterpreter();
    returnValue = interpreter->execute(*this, method->getCode(), 0);
  }

  mCallStack.pop_back();
  if (mCurrentException != nullptr) {
    if (current != nullptr) {
      current->clearOperandStack();
      current->pushOperand(Value::Reference(mCurrentException));
    } else {
      Instance* handler = mThreadInstance->getFieldValue(u"uncaughtExceptionHandler").asReference();
      auto handlerMethod = handler->getClass()->getVirtualMethod(u"uncaughtException", u"(Ljava/lang/Thread;Ljava/lang/Throwable;)V");
      assert(handlerMethod.has_value());

      // FIXME: We should execute the handler, but we do not support `System.err` / `System.out` yet.
      // this->executeCall((*handlerMethod), {Value::Reference(handler), Value::Reference(mThreadInstance), Value::Reference(mCurrentException)});
      Instance* exceptionMessage = mCurrentException->getFieldValue(u"detailMessage").asReference();
      assert(exceptionMessage->getClass()->className() == u"java/lang/String");

      types::JString message = u"Exception ";

      auto exceptionClsName = mCurrentException->getClass()->className();
      std::ranges::replace(exceptionClsName, u'/', u'.');

      message += exceptionClsName;
      message += u": '";
      for (Value ch : exceptionMessage->getFieldValue(u"value").asReference()->asArrayInstance()->contents()) {
        message += ch.asChar();
      }
      message += u"'";

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
    assert(false && "TODO Unknown native method");
  }

  return (*handle)(*this, frame, args);
}

void JavaThread::throwException(Instance* exceptionInstance)
{
  assert(mCurrentException == nullptr && "There is already an exception instance");
  mCurrentException = exceptionInstance;
  currentFrame().clearOperandStack();
  currentFrame().pushOperand(Value::Reference(exceptionInstance));
}

void JavaThread::throwException(const types::JString& name, const types::JString& message)
{
  auto klass = mVm.resolveClass(name);
  if (!klass) {
    assert(false && "TODO: failure to resolve exception class");
  }

  Instance* exceptionInstance = heap().allocate((*klass)->asInstanceClass());
  Instance* messageInstance = heap().intern(message);
  exceptionInstance->setFieldValue(u"detailMessage", Value::Reference(messageInstance));

  this->throwException(exceptionInstance);
}

void JavaThread::clearException()
{
  assert(mCurrentException != nullptr && "There should be an exception instance");
  mCurrentException = nullptr;
}

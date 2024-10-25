#include "Vm.h"
#include "vm/Frame.h"

#include <class_file/Attributes.h>
#include <iostream>

using namespace geevm;

void Vm::initialize()
{
  this->registerNatives();

  this->resolveClass(u"java/lang/Object");
  this->resolveClass(u"java/lang/Class");
  this->resolveClass(u"java/lang/String");

  // auto threadCls = this->resolveClass(u"java/lang/Thread");
  // auto threadGroupCls = this->resolveClass(u"java/lang/ThreadGroup");
  //
  // mCurrentThread = std::make_unique<Instance>(*threadCls);
  // mCurrentThread->setFieldValue(u"priority", Value::Int(1));
  //
  // auto mainThreadGroup = this->newInstance((*threadGroupCls)->asInstanceClass());
  // auto threadGroupCtor = (*threadGroupCls)->getMethod(u"<init>", u"(Ljava/lang/String;)V");
  // // this->execute(threadGroupCtor->klass, threadGroupCtor->method, {Value::Reference(mainThreadGroup), Value::Reference(mInternedStrings.intern(u"main"))});
  //
  // auto threadCtor = (*threadCls)->getMethod(u"<init>", u"(Ljava/lang/ThreadGroup;Ljava/lang/String;)V");
  // this->execute(threadCtor->klass, threadCtor->method,
  //               {Value::Reference(mCurrentThread.get()), Value::Reference(mainThreadGroup), Value::Reference(mInternedStrings.intern(u"init"))});
  //
  // auto systemCls = this->resolveClass(u"java/lang/System");
  // auto initMethod = (*systemCls)->getMethod(u"initializeSystemClass", u"()V");
  //
  // (*systemCls)->storeStaticField(u"lineSeparator", Value::Reference(mInternedStrings.intern(u"\n")));
  //
  // auto fisCls = this->resolveClass(u"java/io/FileInputStream");
  // auto fosCls = this->resolveClass(u"java/io/FileOutputStream");
  // auto printStreamCls = this->resolveClass(u"java/io/PrintStream");
  //
  // auto fdCls = this->resolveClass(u"java/io/FileDescriptor");
  //
  // auto fdCtor = (*fdCls)->getMethod(u"<init>", u"(I)V");
  // auto outFd = this->newInstance((*fdCls)->asInstanceClass());
  //
  // this->execute((*fdCls)->asInstanceClass(), fdCtor->method, std::vector<Value>{Value::Reference(outFd), Value::Int(1)});
  //
  // auto fosCtor = (*fosCls)->getMethod(u"<init>", u"(I)V");
  // auto outFos = this->newInstance((*fosCls)->asInstanceClass());
  //
  // this->execute(fosCtor->klass, fosCtor->method, {Value::Reference(outFos), Value::Reference(outFd)});
  //
  // auto outPs = this->newInstance((*printStreamCls)->asInstanceClass());
  // auto outPsCtor = (*printStreamCls)->getMethod(u"<init>", u"(Ljava/io/OutputStream;Z)V");
  //
  // this->execute(outPsCtor->klass, outPsCtor->method, {Value::Reference(outPs), Value::Reference(outFos), Value::Int(1)});
  //
  // (*systemCls)->storeStaticField(u"out", Value::Reference(outPs));

  // this->execute((*systemCls)->asInstanceClass(), initMethod->method);
}

JMethod* Vm::resolveStaticMethod(JClass* klass, const types::JString& name, const types::JString& descriptor)
{
  // TODO: Check superclasses
  return klass->getMethod(name, descriptor)->method;
}

JMethod* Vm::resolveMethod(JClass* klass, const types::JString& name, const types::JString& descriptor)
{
  return klass->getMethod(name, descriptor)->method;
}

JvmExpected<JClass*> Vm::resolveClass(const types::JString& name)
{
  return mBootstrapClassLoader.loadClass(name);
}

void Vm::execute(InstanceClass* klass, JMethod* method, const std::vector<Value>& args)
{
  CallFrame* current = mCallStack.empty() ? nullptr : &mCallStack.back();
  CallFrame& frame = mCallStack.emplace_back(klass, method, current);
  for (int i = 0; i < args.size(); ++i) {
    frame.storeValue(i, args[i]);
  }

  auto interpreter = createDefaultInterpreter();
  interpreter->execute(*this, method->getCode(), 0);
}

void Vm::executeNative(InstanceClass* klass, JMethod* method, const std::vector<Value>& args)
{
  auto nativeMethod = mNativeMethods.get(ClassNameAndDescriptor{klass->className(), method->name(), method->rawDescriptor()});
  if (nativeMethod) {
    CallFrame* current = mCallStack.empty() ? nullptr : &mCallStack.back();
    auto& newFrame = mCallStack.emplace_back(klass, method, current);
    auto retVal = (*nativeMethod)(*this, newFrame, args);
    mCallStack.pop_back();
    assert(retVal.has_value() || method->descriptor().returnType().isVoid());
    if (retVal.has_value()) {
      mCallStack.back().pushOperand(*retVal);
    }
  } else {
    std::cerr << "Tried to invoke native method "
              << types::convertJString(klass->className() + u"#" + types::JString{method->name()} + types::JString{method->rawDescriptor()}) << "\n";
    assert(false && "Unknown native method!");
  }
}

void Vm::invoke(InstanceClass* klass, JMethod* method)
{
  size_t numArgs = method->descriptor().parameters().size();

  auto& prevFrame = mCallStack.back();

  std::vector<Value> args;
  for (int i = 0; i < numArgs; i++) {
    Value value = prevFrame.popOperand();
    args.push_back(value);
    if (value.isCategoryTwo()) {
      args.push_back(value);
    }
  }

  args.push_back(prevFrame.popOperand());
  std::reverse(args.begin(), args.end());

  if (method->isNative()) {
    this->executeNative(klass, method, args);
  } else {
    this->execute(klass, method, args);
  }
}

void Vm::invokeStatic(InstanceClass* klass, JMethod* method)
{
  size_t numArgs = method->descriptor().parameters().size();
  auto methodName = method->name();
  if (method->isNative() && methodName == u"__geevm_print") {
    auto& prevFrame = mCallStack.back();
    std::vector<Value> arguments;
    for (types::u2 i = 0; i < numArgs; i++) {
      auto value = prevFrame.popOperand();
      arguments.push_back(value);
    }

    auto typeToPrint = method->descriptor().parameters().at(0);
    if (typeToPrint == FieldType{PrimitiveType::Int}) {
      std::cout << arguments[0].asInt() << std::endl;
    } else if (typeToPrint == FieldType{PrimitiveType::Long}) {
      std::cout << arguments[0].asLong() << std::endl;
    } else if (typeToPrint == FieldType{u"java/lang/String"}) {
      Value value = arguments[0].asReference()->getFieldValue(u"value");
      auto& vec = value.asReference()->asArrayInstance()->contents();
      types::JString out;
      for (Value v : vec) {
        out += v.asChar();
      }
      std::cout << types::convertJString(out) << std::endl;
    }
  } else {
    auto& prevFrame = mCallStack.back();

    std::vector<Value> args;
    args.reserve(numArgs);
    for (size_t i = 0; i < numArgs; i++) {
      auto value = prevFrame.popOperand();
      args.push_back(value);
    }

    std::reverse(args.begin(), args.end());

    if (method->isNative()) {
      this->executeNative(klass, method, args);
    } else {
      this->execute(klass, method, args);
    }
  }
}

Instance* Vm::newInstance(InstanceClass* klass)
{
  return mHeap.emplace_back(std::make_unique<Instance>(klass)).get();
}

ArrayInstance* Vm::newArrayInstance(ArrayClass* arrayClass, size_t length)
{
  auto& inserted = mHeap.emplace_back(std::make_unique<ArrayInstance>(arrayClass, length));
  return static_cast<ArrayInstance*>(inserted.get());
}

void Vm::returnToCaller()
{
  mCallStack.pop_back();
}

void Vm::returnToCaller(Value returnValue)
{
  mCallStack.pop_back();
  currentFrame().pushOperand(returnValue);
}

void Vm::unwindToCaller(Instance* instance)
{
  mCallStack.pop_back();
  if (mCallStack.empty()) {
    std::cerr << "Uncaught exception of type " << types::convertJString(instance->getClass()->className()) << std::endl;
    return;
  }

  currentFrame().throwException(instance);
}

void Vm::raiseError(VmError& error)
{
  // TODO
  throw std::logic_error("Exception caught: " + types::convertJString(error.message()));
}

void Vm::raiseError(const types::JString& className)
{
  // TODO
  throw std::logic_error("Exception caught: " + types::convertJString(className));
}

CallFrame& Vm::currentFrame()
{
  return mCallStack.back();
}

Instance* Vm::currentThread()
{
  return mCurrentThread.get();
}

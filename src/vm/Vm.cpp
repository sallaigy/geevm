#include "Vm.h"

#include <iostream>

using namespace geevm;

void Vm::initialize()
{
}

JMethod* Vm::resolveStaticMethod(JClass* klass, const types::JString& name, const types::JString& descriptor)
{
  // TODO: Check superclasses
  return klass->getMethod(name, descriptor);
}

JMethod* Vm::resolveMethod(JClass* klass, const types::JString& name, const types::JString& descriptor)
{
  return klass->getMethod(name, descriptor);
}

JvmExpected<JClass*> Vm::resolveClass(const types::JString& name)
{
  auto klass = mBootstrapClassLoader.loadClass(name);
  if (klass) {
    if (auto superClass = (*klass)->superClass(); superClass) {
      this->resolveClass(types::JString{*superClass});
    }

    for (types::JStringRef interfaceName : (*klass)->interfaces()) {
      this->resolveClass(types::JString{interfaceName});
    }

    (*klass)->prepare();
    (*klass)->initialize(*this);
  }
  return klass;
}

void Vm::execute(JClass* klass, JMethod* method)
{
  mCallStack.emplace_back(klass, method);
  auto interpreter = createDefaultInterpreter();
  interpreter->execute(*this, method->getCode(), 0);
}

void Vm::invoke(JClass* klass, JMethod* method)
{
  size_t numArgs = method->getDescriptor().parameters().size();

  if (hasAccessFlag(method->getMethodInfo().accessFlags(), MethodAccessFlags::ACC_NATIVE)) {
    auto methodName = klass->constantPool().getString(method->getMethodInfo().nameIndex());
    if (methodName == u"__geevm_print") {
      auto& prevFrame = mCallStack.back();
      std::vector<Value> arguments;
      for (types::u2 i = 0; i < numArgs; i++) {
        auto value = prevFrame.popOperand();
        arguments.push_back(value);
      }

      auto typeToPrint = method->getDescriptor().parameters().at(0);
      if (typeToPrint == FieldType{PrimitiveType::Int}) {
        std::cout << arguments[0].asInt() << std::endl;
      } else if (typeToPrint == FieldType{PrimitiveType::Long}) {
        std::cout << arguments[0].asLong() << std::endl;
      }
    }
  } else {
    auto& newFrame = mCallStack.emplace_back(klass, method);
    auto& prevFrame = mCallStack.at(mCallStack.size() - 2);

    for (types::u2 i = 0; i < numArgs; i++) {
      auto value = prevFrame.popOperand();
      newFrame.storeValue(numArgs - 1 - i, value);
    }

    auto interpreter = createDefaultInterpreter();
    interpreter->execute(*this, method->getCode(), 0);
  }
}

Instance* Vm::newInstance(JClass* klass)
{
  // TODO: Fields, etc.
  auto& inserted = mHeap.emplace_back(std::make_unique<Instance>(klass));

  return inserted.get();
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

void Vm::raiseError(VmError& error)
{
  std::cerr << "Exception caught: " << types::convertJString(error.message()) << std::endl;
}

CallFrame& Vm::currentFrame()
{
  return mCallStack.back();
}

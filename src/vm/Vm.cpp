#include "Vm.h"

using namespace geevm;

JMethod* Vm::resolveStaticMethod(JClass* klass, const types::JString& name, const types::JString& descriptor)
{
  // TODO: Check superclasses
  return klass->getMethod(name, descriptor);
}

void Vm::execute(JClass* klass, JMethod* method)
{
  mCallStack.emplace_back(klass, method);
  auto interpreter = createDefaultInterpreter();
  interpreter->execute(*this, method->getCode(), 0);
}

void Vm::raiseError(VmError& error)
{

}

CallFrame& Vm::currentFrame()
{
  return mCallStack.back();
}

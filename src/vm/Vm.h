#ifndef GEEVM_VM_VM_H
#define GEEVM_VM_VM_H

#include <unordered_map>
#include <utility>

#include "common/JvmError.h"
#include "vm/Class.h"
#include "vm/ClassLoader.h"
#include "vm/Frame.h"
#include "vm/Interpreter.h"
#include "vm/Method.h"

namespace geevm
{

class Vm
{
public:
  JvmExpected<JClass*> resolveClass(const types::JString& name);

  void execute(JClass* klass, JMethod* method);

  void invoke(JClass* klass, JMethod* method);

  void returnToCaller();

  void returnToCaller(Value returnValue);

  JMethod* resolveStaticMethod(JClass* klass, const types::JString& name, const types::JString& descriptor);

  void raiseError(VmError& error);

  CallFrame& currentFrame();

private:
  ClassLoader mBootstrapClassLoader;
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mLoadedClasses;
  std::vector<CallFrame> mCallStack;
  // Method area
};

} // namespace geevm

#endif // GEEVM_VM_VM_H

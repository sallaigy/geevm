#ifndef GEEVM_VM_VM_H
#define GEEVM_VM_VM_H

#include "common/JvmError.h"

#include "vm/Class.h"
#include "vm/Method.h"
#include "vm/ClassLoader.h"
#include "vm/Frame.h"
#include "vm/Interpreter.h"

#include <unordered_map>
#include <utility>

namespace geevm
{

class Vm
{
public:
  JvmExpected<JClass*> resolveClass(const types::JString& name)
  {
    return mBootstrapClassLoader.loadClass(name);
  }

  void execute(JClass* klass, JMethod* method);

  JMethod* resolveStaticMethod(JClass* klass, const types::JString& name, const types::JString& descriptor);

  void raiseError(VmError& error);

  CallFrame& currentFrame();

private:
  ClassLoader mBootstrapClassLoader;
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mLoadedClasses;
  std::vector<CallFrame> mCallStack;
};

}

#endif //GEEVM_VM_VM_H

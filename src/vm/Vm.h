#ifndef GEEVM_VM_VM_H
#define GEEVM_VM_VM_H

#include <unordered_map>

#include "common/JvmError.h"
#include "vm/Class.h"
#include "vm/ClassLoader.h"
#include "vm/Frame.h"
#include "vm/Interpreter.h"
#include "vm/Method.h"
#include "vm/StringHeap.h"

namespace geevm
{

class Vm
{
public:
  explicit Vm()
    : mInternedStrings(*this), mBootstrapClassLoader(*this)
  {
  }

  JvmExpected<JClass*> resolveClass(const types::JString& name);
  JvmExpected<ArrayClass*> resolveArrayClass(const types::JString& name);

  void initialize();

  void execute(JClass* klass, JMethod* method);

  void invoke(JClass* klass, JMethod* method);

  void returnToCaller();

  void returnToCaller(Value returnValue);

  JMethod* resolveStaticMethod(JClass* klass, const types::JString& name, const types::JString& descriptor);

  JMethod* resolveMethod(JClass* klass, const types::JString& name, const types::JString& descriptor);

  void raiseError(VmError& error);

  CallFrame& currentFrame();

  Instance* newInstance(JClass* klass);
  ArrayInstance* newArrayInstance(ArrayClass* arrayClass, size_t size);

  StringHeap& internedStrings()
  {
    return mInternedStrings;
  }

private:
  BootstrapClassLoader mBootstrapClassLoader;
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mLoadedClasses;
  std::vector<CallFrame> mCallStack;
  // Method area
  std::vector<std::unique_ptr<Instance>> mHeap;
  StringHeap mInternedStrings;
};

} // namespace geevm

#endif // GEEVM_VM_VM_H

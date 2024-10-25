#ifndef GEEVM_VM_VM_H
#define GEEVM_VM_VM_H

#include <unordered_map>

#include "common/JvmError.h"
#include "vm/Class.h"
#include "vm/ClassLoader.h"
#include "vm/Frame.h"
#include "vm/Interpreter.h"
#include "vm/Method.h"
#include "vm/NativeMethods.h"
#include "vm/StringHeap.h"

#include <list>

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

  void initialize();

  void execute(InstanceClass* klass, JMethod* method, const std::vector<Value>& args = {});
  void executeNative(InstanceClass* klass, JMethod* method, const std::vector<Value>& args);

  void invoke(InstanceClass* klass, JMethod* method);
  void invokeStatic(InstanceClass* klass, JMethod* method);

  void returnToCaller();

  void returnToCaller(Value returnValue);
  void unwindToCaller(Instance* instance);

  JMethod* resolveStaticMethod(JClass* klass, const types::JString& name, const types::JString& descriptor);

  JMethod* resolveMethod(JClass* klass, const types::JString& name, const types::JString& descriptor);

  void raiseError(VmError& error);
  void raiseError(const types::JString& className);

  CallFrame& currentFrame();
  Instance* currentThread();

  Instance* newInstance(InstanceClass* klass);
  ArrayInstance* newArrayInstance(ArrayClass* arrayClass, size_t size);

  StringHeap& internedStrings()
  {
    return mInternedStrings;
  }

  NativeMethodRegistry& nativeMethods()
  {
    return mNativeMethods;
  }

private:
  void registerNatives();

private:
  BootstrapClassLoader mBootstrapClassLoader;
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mLoadedClasses;
  NativeMethodRegistry mNativeMethods;
  // Stack
  std::list<CallFrame> mCallStack;
  // TODO: We only support one thread
  std::unique_ptr<Instance> mCurrentThread;
  // Method area
  std::vector<std::unique_ptr<Instance>> mHeap;
  StringHeap mInternedStrings;
};

} // namespace geevm

#endif // GEEVM_VM_VM_H

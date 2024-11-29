#ifndef GEEVM_VM_VM_H
#define GEEVM_VM_VM_H

#include <unordered_map>

#include "common/JvmError.h"
#include "vm/Class.h"
#include "vm/ClassLoader.h"
#include "vm/Heap.h"
#include "vm/Interpreter.h"
#include "vm/NativeMethods.h"
#include "vm/Thread.h"

namespace geevm
{

class Vm
{
public:
  explicit Vm()
    : mBootstrapClassLoader(*this), mMainThread(*this), mHeap(*this)
  {
  }

  JvmExpected<JClass*> resolveClass(const types::JString& name);

  void initialize();

  JavaHeap& heap()
  {
    return mHeap;
  }

  NativeMethodRegistry& nativeMethods()
  {
    return mNativeMethods;
  }

  JavaThread& mainThread()
  {
    return mMainThread;
  }

  BootstrapClassLoader& bootstrapClassLoader()
  {
    return mBootstrapClassLoader;
  }

private:
  void registerNatives();

  /// Resolves and initializes a core class
  JClass* requireClass(const types::JString& name);

private:
  BootstrapClassLoader mBootstrapClassLoader;
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mLoadedClasses;
  NativeMethodRegistry mNativeMethods;
  JavaHeap mHeap;
  // TODO: We only support one thread
  JavaThread mMainThread;
};

} // namespace geevm

#endif // GEEVM_VM_VM_H

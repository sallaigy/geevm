#ifndef GEEVM_VM_VM_H
#define GEEVM_VM_VM_H

#include <unordered_map>

#include "common/JvmError.h"
#include "vm/Class.h"
#include "vm/ClassLoader.h"
#include "vm/Frame.h"
#include "vm/Heap.h"
#include "vm/Interpreter.h"
#include "vm/Method.h"
#include "vm/NativeMethods.h"
#include "vm/StringHeap.h"
#include "vm/Thread.h"

#include <list>

namespace geevm
{

class Vm
{
public:
  explicit Vm()
    : mMainThread(*this), mHeap(*this), mBootstrapClassLoader(*this)
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

#ifndef GEEVM_VM_VM_H
#define GEEVM_VM_VM_H

#include "common/JvmError.h"
#include "vm/Class.h"
#include "vm/ClassLoader.h"
#include "vm/Heap.h"
#include "vm/NativeMethods.h"
#include "vm/Thread.h"
#include "vm/jit/JitCompiler.h"

#include <ranges>
#include <unordered_map>

namespace geevm
{

class JitCompiler;

struct VmSettings
{
  bool runGcAfterEveryAllocation = false;
  bool noSystemInit = false;
  size_t maxHeapSize = 2 * 2048l * 1024;
  size_t maxStackSize = 1024l * 1024;
  std::string javaHome = "";
  std::unordered_set<std::string> jitFunctions;
  bool jitAllFunctions = false;
};

class Vm
{
public:
  explicit Vm(VmSettings settings);

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
    return *mMainThread;
  }

  BootstrapClassLoader& bootstrapClassLoader()
  {
    return mBootstrapClassLoader;
  }

  auto threads()
  {
    return mThreads | std::views::transform([](std::unique_ptr<JavaThread>& ptr) {
      return ptr.get();
    });
  }

  const VmSettings& settings()
  {
    return mSettings;
  }

  JitCompiler& jit() const
  {
    return *mJitCompiler;
  }

  ~Vm() = default;

private:
  /// Resolves and initializes a core class
  JClass* requireClass(const types::JString& name);
  void setUpJavaLangClass();

private:
  VmSettings mSettings;
  BootstrapClassLoader mBootstrapClassLoader;
  std::unordered_map<types::JString, std::unique_ptr<JClass>> mLoadedClasses;
  NativeMethodRegistry mNativeMethods;
  JavaHeap mHeap;
  std::unique_ptr<JitCompiler> mJitCompiler;

  // TODO: We only support one thread
  JavaThread* mMainThread = nullptr;
  std::vector<std::unique_ptr<JavaThread>> mThreads;
};

} // namespace geevm

#endif // GEEVM_VM_VM_H

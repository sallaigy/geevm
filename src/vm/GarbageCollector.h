#ifndef GEEVM_VM_GARBAGECOLLECTOR_H
#define GEEVM_VM_GARBAGECOLLECTOR_H

#include "common/JvmError.h"

#include <cstddef>
#include <list>
#include <unordered_map>

namespace geevm
{

class Vm;
class Instance;

/// A special reference that marks the object it points to as a GC root.
/// As the garbage collector may relocate memory, this reference provides safe access to an object, even if the GC
/// decides to relocate the object it refers to.
template<class T = Instance>
  requires(std::is_base_of_v<Instance, T>)
class GcRootRef
{
public:
  explicit GcRootRef(T** root)
    : mReference(root)
  {
  }

  GcRootRef(const GcRootRef& other) = default;
  GcRootRef(GcRootRef&& other) = default;

  T* operator->() const
  {
    return *mReference;
  }

  T* operator*() const
  {
    return *mReference;
  }

private:
  T** mReference;
};

class GarbageCollector
{
public:
  explicit GarbageCollector(Vm& vm, size_t heapSize);

  [[nodiscard]] void* allocate(size_t size);

  void performGarbageCollection();

  GcRootRef<Instance> pin(Instance* object);

  // Locks the garbage collector, preventing it from running.
  void lockGC();

  // Unlocks the garbage collector, allowing it to run.
  void unlockGC();

  ~GarbageCollector();

private:
  Instance* copyObject(Instance* instance, std::unordered_map<Instance*, Instance*>& map);

private:
  Vm& mVm;
  // Garbage-collected heap
  size_t mHeapSize;
  char* mFromRegion;
  char* mToRegion;
  char* mBumpPtr;
  // Enabling/disabling GC
  bool mIsGcRunning = false;
  // Others
  std::list<Instance*> mRootList;
};

} // namespace geevm

#endif // GEEVM_VM_GARBAGECOLLECTOR_H

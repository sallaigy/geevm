#ifndef GEEVM_VM_GARBAGECOLLECTOR_H
#define GEEVM_VM_GARBAGECOLLECTOR_H

#include "common/JvmError.h"

#include <cassert>
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
template<std::derived_from<Instance> T = Instance>
class GcRootRef
{
  template<std::derived_from<Instance> U>
  friend class GcRootRef;

public:
  explicit GcRootRef(Instance** root)
    : mReference(root)
  {
  }

  /*implicit*/ GcRootRef(std::nullptr_t)
    : mReference(nullptr)
  {
  }

  GcRootRef(const GcRootRef& other) = default;
  GcRootRef& operator=(const GcRootRef& other) = default;
  GcRootRef(GcRootRef&& other) = default;
  GcRootRef& operator=(GcRootRef&& other) = default;

  template<std::derived_from<T> U>
  GcRootRef(const GcRootRef<U>& other)
    : mReference(other.mReference)
  {
  }

  T* operator->() const
  {
    return static_cast<T*>(*mReference);
  }

  T* get() const
  {
    if (mReference == nullptr) {
      return nullptr;
    }
    return static_cast<T*>(*mReference);
  }

  bool operator==(GcRootRef<T> other) const
  {
    return mReference == other.mReference;
  }

  bool operator==(std::nullptr_t) const
  {
    return mReference == nullptr;
  }

  void reset()
  {
    mReference = nullptr;
  }

private:
  Instance** mReference;
};

class GarbageCollector
{
public:
  explicit GarbageCollector(Vm& vm, size_t heapSize);

  [[nodiscard]] void* allocate(size_t size);

  void performGarbageCollection();

  template<std::derived_from<Instance> T>
  GcRootRef<T> pin(T* object)
  {
    if (object == nullptr) {
      return nullptr;
    }

    Instance** root = &mRootList.emplace_back(object);
    return GcRootRef<T>(root);
  }

  template<std::derived_from<Instance> T>
  void release(GcRootRef<T> object)
  {
    if (object == nullptr) {
      return;
    }

    mRootList.remove_if([&object](Instance* val) {
      return object.get() == val;
    });
  }

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

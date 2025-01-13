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

class RootList
{
public:
  struct Node
  {
    Instance* instance;
    Node* next;
    Node* prev = nullptr;

    Node(Instance* instance, Node* next, Node* prev)
      : instance(instance), next(next), prev(prev)
    {
    }
  };

  struct Iterator
  {
    Node* node;

    explicit Iterator(Node* node)
      : node(node)
    {
    }

    bool operator==(const Iterator& other) const
    {
      return node == other.node;
    }

    Iterator& operator=(const Iterator& rhs)
    {
      node = rhs.node;
      return *this;
    }

    Iterator& operator++()
    {
      node = node->next;
      return *this;
    }

    Instance*& operator*()
    {
      return node->instance;
    }

    Instance*& operator->()
    {
      return node->instance;
    }
  };

  RootList() = default;

  Node* insert(Instance* reference);
  void remove(Node* node);

  ~RootList();

  Iterator begin()
  {
    return Iterator(mHead);
  }

  Iterator end()
  {
    return Iterator(nullptr);
  }

private:
  Node* mHead = nullptr;
};

/// A special reference that marks the object it points to as a GC root.
/// As the garbage collector may relocate memory, this reference provides safe access to an object, even if the GC
/// decides to relocate the object it refers to.
template<std::derived_from<Instance> T = Instance>
class GcRootRef
{
  template<std::derived_from<Instance> U>
  friend class GcRootRef;

  friend class GarbageCollector;

public:
  explicit GcRootRef(RootList::Node* root)
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
    return static_cast<T*>(mReference->instance);
  }

  T* get() const
  {
    if (mReference == nullptr) {
      return nullptr;
    }
    return static_cast<T*>(mReference->instance);
  }

  bool operator==(GcRootRef<T> other) const
  {
    if (mReference == nullptr || other.mReference == nullptr) {
      return mReference == other.mReference;
    }

    return mReference->instance == other.mReference->instance;
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
  RootList::Node* mReference;
};

class GarbageCollector
{
public:
  explicit GarbageCollector(Vm& vm);

  [[nodiscard]] void* allocate(size_t size);

  void performGarbageCollection();

  template<std::derived_from<Instance> T>
  GcRootRef<T> pin(T* object)
  {
    if (object == nullptr) {
      return nullptr;
    }

    RootList::Node* root = mRootList.insert(object);
    return GcRootRef<T>(root);
  }

  template<std::derived_from<Instance> T>
  void release(GcRootRef<T> object)
  {
    if (object == nullptr) {
      return;
    }

    mRootList.remove(object.mReference);
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
  char* mFromRegion;
  char* mToRegion;
  char* mBumpPtr;
  // Enabling/disabling GC
  bool mIsGcRunning = false;
  // Root lists
  RootList mRootList;
  // GC settings
  size_t mHeapSize = 0;
  bool mRunAfterEveryAllocation = false;
};

} // namespace geevm

#endif // GEEVM_VM_GARBAGECOLLECTOR_H

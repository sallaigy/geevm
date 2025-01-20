#ifndef GEEVM_VM_GARBAGECOLLECTOR_H
#define GEEVM_VM_GARBAGECOLLECTOR_H

#include "vm/Instance.h"

#include <cassert>
#include <cstddef>
#include <list>
#include <unordered_map>

namespace geevm
{

class Vm;
class Instance;
class GarbageCollector;

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

protected:
  explicit GcRootRef(RootList::Node* root)
    : mReference(root)
  {
  }

public:
  /*implicit*/ GcRootRef(std::nullptr_t)
    : mReference(nullptr)
  {
  }

  GcRootRef(const GcRootRef& other) = default;
  GcRootRef& operator=(const GcRootRef& other) = default;
  GcRootRef(GcRootRef&& other) = default;
  GcRootRef& operator=(GcRootRef&& other) = default;

  template<std::derived_from<T> U>
  /*implicit*/ GcRootRef(const GcRootRef<U>& other)
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

  bool operator==(const Instance* other) const
  {
    if (mReference == nullptr) {
      return other == nullptr;
    }

    return mReference->instance == other;
  }

  void reset()
  {
    mReference = nullptr;
  }

protected:
  RootList::Node* mReference;
};

/// A scoped, owning version of a GcRootRef. When this reference goes out of scope, the referenced object is unpinned by the garbage collector.
template<std::derived_from<Instance> T = Instance>
class ScopedGcRootRef : private GcRootRef<T>
{
  friend class GarbageCollector;

  ScopedGcRootRef(RootList::Node* root, GarbageCollector* gc)
    : GcRootRef<T>(root), mGC(gc)
  {
    assert(mGC != nullptr);
  }

public:
  ScopedGcRootRef(const ScopedGcRootRef&) = delete;
  ScopedGcRootRef& operator=(const ScopedGcRootRef&) = delete;

  ScopedGcRootRef(ScopedGcRootRef&& other) noexcept
    : GcRootRef<T>(other.release()), mGC(other.mGC)
  {
  }
  ScopedGcRootRef& operator=(ScopedGcRootRef&& other) noexcept
  {
    this->~ScopedGcRootRef();
    this = new (this) ScopedGcRootRef(std::move(other));
    return *this;
  }

  using GcRootRef<T>::operator->;
  using GcRootRef<T>::operator==;
  using GcRootRef<T>::get;

  /// Releases ownership of the GC root held by this reference, returning a non-owning GC root ref.
  /// After calling this method, it is the responsibility of the caller to free the given reference.
  [[nodiscard]] GcRootRef<T> release()
  {
    GcRootRef<T> copy(*this);
    this->reset();
    return copy;
  }

  ~ScopedGcRootRef();

private:
  GarbageCollector* mGC;
};

class GarbageCollector
{
public:
  explicit GarbageCollector(Vm& vm);

  [[nodiscard]] void* allocate(size_t size);

  void performGarbageCollection();

  /// Marks the given object as a GC root. The return value of this function is a special reference that
  /// is GC-safe and is not invalidated when the GC relocates the pointed object.
  template<std::derived_from<Instance> T>
  ScopedGcRootRef<T> pin(T* object)
  {
    if (object == nullptr) {
      return ScopedGcRootRef<T>(nullptr, this);
    }

    RootList::Node* root = mRootList.insert(object);
    return ScopedGcRootRef<T>(root, this);
  }

  /// Releases a given root reference. All other root references are invalidated.
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

template<std::derived_from<Instance> T>
ScopedGcRootRef<T>::~ScopedGcRootRef()
{
  mGC->release(*this);
}

} // namespace geevm

#endif // GEEVM_VM_GARBAGECOLLECTOR_H

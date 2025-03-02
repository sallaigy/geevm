#ifndef GEEVM_HEAP_H
#define GEEVM_HEAP_H

#include "common/JvmTypes.h"
#include "vm/Class.h"
#include "vm/GarbageCollector.h"
#include "vm/Instance.h"

namespace geevm
{

class Vm;
class JClass;
class Instance;
class InstanceClass;
class ArrayClass;
class ArrayInstance;

/// The garbage-collected Java heap.
/// All allocations of Java objects should happen through this class.
class JavaHeap
{
public:
  explicit JavaHeap(Vm& vm);

  /// Initializes the heap with the classes necessary to perform string interning.
  /// All calls to 'intern' are invalid before the heap has been initialized.
  void initialize(InstanceClass* stringClass, ArrayClass* byteArrayClass);

  /// Allocates heap memory for and constructs an instance of `klass`.
  template<std::derived_from<Instance> T, class... Args>
  T* allocate(InstanceClass* klass, Args&&... args)
  {
    size_t size = klass->allocationSize();
    void* mem = mGC.allocate(size);

    auto object = new (mem) T(klass, std::forward<Args>(args)...);
    return object;
  }

  /// Allocates heap memory for an array instance of the array class 'klass' of a given length.
  template<JvmType T>
  JavaArray<T>* allocateArray(ArrayClass* klass, int32_t length)
  {
    assert(length >= 0);

    size_t size = klass->allocationSize(length);
    void* mem = mGC.allocate(size);

    auto array = new (mem) JavaArray<T>(klass, length);
    return array;
  }

  ArrayInstance* allocateArray(ArrayClass* klass, int32_t length);

  /// Interns the given string.
  /// If this method constructs a new string object, it will be pinned by the garbage collector and kept alive
  /// until the heap is destroyed.
  GcRootRef<> intern(const types::JString& string);

  GarbageCollector& gc()
  {
    return mGC;
  }

private:
  Vm& mVm;
  // Garbage-collected heap
  GarbageCollector mGC;
  // Interned strings, including classes that need to present for string interning
  std::unordered_map<types::JString, GcRootRef<>> mInternedStrings;
  InstanceClass* mStringClass = nullptr;
  ArrayClass* mByteArrayClass = nullptr;
};

} // namespace geevm

#endif // GEEVM_HEAP_H

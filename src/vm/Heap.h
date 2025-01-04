#ifndef GEEVM_HEAP_H
#define GEEVM_HEAP_H

#include "common/JvmTypes.h"
#include "vm/Class.h"
#include "vm/Frame.h"
#include "vm/GarbageCollector.h"
#include "vm/Instance.h"
#include "vm/StringHeap.h"

namespace geevm
{

class Vm;
class JClass;
class Instance;
class InstanceClass;
class ArrayClass;
class ArrayInstance;
class StringHeap;

class JavaHeap
{
public:
  explicit JavaHeap(Vm& vm);

  template<class T = Instance, class... Args>
    requires(std::is_base_of_v<Instance, T>)
  T* allocate(InstanceClass* klass, Args&&... args)
  {
    size_t size = klass->allocationSize();
    void* mem = mGC.allocate(size);

    auto object = new (mem) T(klass, std::forward<Args>(args)...);
    return object;
  }

  template<class T = Instance, class... Args>
    requires(std::is_base_of_v<Instance, T>)
  T* allocatePerm(InstanceClass* klass, Args&&... args)
  {
    size_t size = klass->allocationSize();
    void* mem = this->allocateSpaceOnPerm(size);

    auto object = new (mem) T(klass, std::forward<Args>(args)...);
    return object;
  }

  template<JvmType T>
  JavaArray<T>* allocateArray(ArrayClass* klass, int32_t length)
  {
    assert(length >= 0);

    size_t size = klass->allocationSize(length);
    void* mem = mGC.allocate(size);

    auto array = new (mem) JavaArray<T>(klass, length);
    return array;
  }

  template<JvmType T>
  JavaArray<T>* allocateArrayOnPerm(ArrayClass* klass, int32_t length)
  {
    assert(length >= 0);

    size_t size = klass->allocationSize(length);
    void* mem = this->allocateSpaceOnPerm(size);

    auto array = new (mem) JavaArray<T>(klass, length);
    return array;
  }

  ArrayInstance* allocateArray(ArrayClass* klass, int32_t length);

  Instance* intern(const types::JString& utf8)
  {
    return mInternedStrings.intern(utf8);
  }

  StringHeap& stringHeap()
  {
    return mInternedStrings;
  }

  GarbageCollector& gc()
  {
    return mGC;
  }

  ~JavaHeap();

private:
  void* allocateSpaceOnPerm(size_t size);

private:
  StringHeap mInternedStrings;
  // Garbage-collected heap
  GarbageCollector mGC;
  // Permanent heap
  char* mPermanentRegion;
  char* mPermanentBumpPtr;
};

} // namespace geevm

#endif // GEEVM_HEAP_H

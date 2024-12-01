#ifndef GEEVM_HEAP_H
#define GEEVM_HEAP_H

#include "common/JvmTypes.h"
#include "vm/Class.h"
#include "vm/Frame.h"
#include "vm/Instance.h"
#include "vm/StringHeap.h"

#include <memory>
#include <vector>

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
  struct ObjectDeleter
  {
    void operator()(Instance* obj)
    {
      if (obj->getClass()->className() == u"java/lang/Class") {
        obj->asClassInstance()->~ClassInstance();
      } else {
        obj->~Instance();
      }
      ::operator delete(obj);
    }
  };

  using ObjectPtr = std::unique_ptr<Instance, ObjectDeleter>;

public:
  explicit JavaHeap(Vm& vm)
    : mInternedStrings(vm)
  {
  }

  template<class T = Instance, class... Args>
    requires(std::is_base_of_v<Instance, T>)
  T* allocate(InstanceClass* klass, Args&&... args)
  {
    size_t numFields = klass->numInstanceFields();
    size_t size = sizeof(T) + numFields * sizeof(Value);
    void* mem = ::operator new(size);

    auto* fieldsStart = reinterpret_cast<Value*>(reinterpret_cast<char*>(mem) + sizeof(T));

    auto object = new (mem) T(klass, fieldsStart, std::forward<Args>(args)...);
    ObjectPtr ptr{object, ObjectDeleter{}};
    return static_cast<T*>(mHeap.emplace_back(std::move(ptr)).get());
  }

  /// Allocates space for an instance of the given class.
  ArrayInstance* allocateArray(ArrayClass* klass, int32_t length);

  Instance* intern(const types::JString& utf8)
  {
    return mInternedStrings.intern(utf8);
  }

  StringHeap& stringHeap()
  {
    return mInternedStrings;
  }

private:
  std::vector<ObjectPtr> mHeap;
  std::vector<std::unique_ptr<ArrayInstance>> mArrayHeap;
  StringHeap mInternedStrings;
};

} // namespace geevm

#endif // GEEVM_HEAP_H

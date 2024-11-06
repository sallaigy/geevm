#ifndef GEEVM_HEAP_H
#define GEEVM_HEAP_H

#include "common/JvmTypes.h"
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
public:
  explicit JavaHeap(Vm& vm)
    : mInternedStrings(vm)
  {
  }

  /// Allocates space for an instance of the given class.
  Instance* allocate(InstanceClass* klass);
  ArrayInstance* allocateArray(ArrayClass* klass, size_t length);

  Instance* intern(types::JStringRef utf8)
  {
    return mInternedStrings.intern(utf8);
  }

  StringHeap& stringHeap()
  {
    return mInternedStrings;
  }

private:
  std::vector<std::unique_ptr<Instance>> mHeap;
  StringHeap mInternedStrings;
};

} // namespace geevm

#endif // GEEVM_HEAP_H

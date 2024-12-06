#include "vm/Heap.h"
#include "vm/Instance.h"

using namespace geevm;

ArrayInstance* JavaHeap::allocateArray(ArrayClass* klass, int32_t length)
{
  assert(length >= 0);

  const auto& inserted = mArrayHeap.emplace_back(ArrayInstance::create(klass, length));
  return static_cast<ArrayInstance*>(inserted.get());
}

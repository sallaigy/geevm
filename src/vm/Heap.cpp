#include "vm/Heap.h"
#include "vm/Instance.h"

using namespace geevm;

Instance* JavaHeap::allocate(InstanceClass* klass)
{
  return mHeap.emplace_back(std::make_unique<Instance>(klass)).get();
}

ArrayInstance* JavaHeap::allocateArray(ArrayClass* klass, int32_t length)
{
  assert(length >= 0);
  const auto& inserted = mHeap.emplace_back(std::make_unique<ArrayInstance>(klass, length));
  return static_cast<ArrayInstance*>(inserted.get());
}

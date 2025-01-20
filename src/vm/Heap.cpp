#include "vm/Heap.h"
#include "vm/Instance.h"

using namespace geevm;

JavaHeap::JavaHeap(Vm& vm)
  : mInternedStrings(vm), mGC(vm)
{
}

ArrayInstance* JavaHeap::allocateArray(ArrayClass* klass, int32_t length)
{
  auto elementType = klass->fieldType().asArrayType()->getElementType();

  return elementType.map([&]<PrimitiveType Type>() -> ArrayInstance* {
    using Representation = typename PrimitiveTypeTraits<Type>::Representation;
    return this->allocateArray<Representation>(klass, length);
  }, [&](types::JStringRef) -> ArrayInstance* {
    return this->allocateArray<Instance*>(klass, length);
  }, [&](const ArrayType&) -> ArrayInstance* {
    return this->allocateArray<Instance*>(klass, length);
  });
}

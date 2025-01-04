#include "vm/Heap.h"
#include "vm/Instance.h"

using namespace geevm;

static constexpr size_t MaxPermRegionSize = 2048 * 1024;

JavaHeap::JavaHeap(Vm& vm)
  : mInternedStrings(vm), mGC(vm, 2048 * 1024)
{
  mPermanentRegion = static_cast<char*>(::operator new(MaxPermRegionSize));
  mPermanentBumpPtr = mPermanentRegion;
}

JavaHeap::~JavaHeap()
{
  ::operator delete(mPermanentRegion);
}

void* JavaHeap::allocateSpaceOnPerm(size_t size)
{
  assert(mPermanentBumpPtr + size <= mPermanentRegion + MaxPermRegionSize);

  char* current = mPermanentBumpPtr;
  mPermanentBumpPtr += size;
  return current;
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

#include "vm/Heap.h"
#include "vm/Instance.h"

using namespace geevm;

JavaHeap::JavaHeap(Vm& vm)
  : mVm(vm), mGC(vm)
{
}

void JavaHeap::initialize(InstanceClass* stringClass, ArrayClass* byteArrayClass)
{
  assert(stringClass->className() == u"java/lang/String");
  assert(byteArrayClass->className() == u"[B");

  mStringClass = stringClass;
  mByteArrayClass = byteArrayClass;
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

GcRootRef<> JavaHeap::intern(const types::JString& string)
{
  if (auto it = mInternedStrings.find(string); it != mInternedStrings.end()) {
    return it->second;
  }

  assert(mStringClass != nullptr);
  assert(mByteArrayClass != nullptr);

  std::vector<uint8_t> bytes;
  for (char16_t c : string) {
    bytes.push_back(c & 0xff);
    bytes.push_back((c >> 8) & 0xff);
  }

  GcRootRef<> newInstance = mGC.pin(this->allocate<ObjectInstance>(mStringClass)).release();
  auto* stringContents = this->allocateArray<int8_t>(mByteArrayClass, bytes.size());
  for (int32_t i = 0; i < bytes.size(); ++i) {
    stringContents->setArrayElement(i, std::bit_cast<int8_t>(bytes[i]));
  }
  newInstance->setFieldValue<Instance*>(u"value", u"[B", stringContents);

  auto [res, _] = mInternedStrings.try_emplace(string, newInstance);
  return res->second;
}

#include "vm/Instance.h"
#include "vm/Class.h"

#include <iostream>

using namespace geevm;

ObjectInstance::ObjectInstance(InstanceClass* klass)
  : mHeader(klass, 0)
{
  for (const auto& [key, field] : klass->fields()) {
    if (!field->isStatic()) {
      auto& fieldType = field->fieldType();
      fieldType.map([&]<PrimitiveType Type>() {
        this->setFieldValue<typename PrimitiveTypeTraits<Type>::Representation>(field->offset(), 0);
      }, [&](types::JStringRef) {
        this->setFieldValue<Instance*>(field->offset(), nullptr);
      }, [&](const ArrayType&) {
        this->setFieldValue<Instance*>(field->offset(), nullptr);
      });
    }
  }
}

size_t Instance::getFieldOffset(types::JStringRef fieldName, types::JStringRef descriptor) const
{
  NameAndDescriptor key{fieldName, descriptor};
  assert(getClass()->fields().contains(key));
  auto& field = getClass()->fields().at(key);

  return field->offset();
}

int32_t Instance::hashCode()
{
  InstanceHeader& header = getHeader();
  if (header.mHashCode == 0) {
    header.mHashCode = static_cast<int32_t>(std::hash<const Instance*>{}(this));
  }

  return header.mHashCode;
}

ArrayInstance::ArrayInstance(ArrayClass* arrayClass, int32_t length)
  : mHeader(arrayClass, 0), mLength(length)
{
}

ArrayInstance* Instance::toArrayInstance()
{
  assert(getClass()->isArrayType());
  return static_cast<ArrayInstance*>(this);
}

ClassInstance* Instance::toClassInstance()
{
  assert(getClass()->className() == u"java/lang/Class");
  return static_cast<ClassInstance*>(this);
}

void JavaString::verify()
{
#ifndef NDEBUG
  auto valueField = getClass()->lookupField(u"value", u"[B");
  assert(offsetof(JavaString, mValue) == (*valueField)->offset());

  auto coderField = getClass()->lookupField(u"coder", u"B");
  assert(offsetof(JavaString, mCoder) == (*coderField)->offset());

  auto hashField = getClass()->lookupField(u"hash", u"I");
  assert(offsetof(JavaString, mHash) == (*hashField)->offset());

  auto hashIsZeroField = getClass()->lookupField(u"hashIsZero", u"Z");
  assert(offsetof(JavaString, mHashIsZero) == (*hashIsZeroField)->offset());
#endif
}

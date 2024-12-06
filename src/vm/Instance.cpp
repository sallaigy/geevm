#include "vm/Instance.h"

#include <iostream>

using namespace geevm;

Instance::Instance(JClass* klass)
  : mClass(klass)
{
  for (const auto& [key, field] : klass->fields()) {
    if (!field->isStatic()) {
      auto& fieldType = field->fieldType();
      fieldType.map(
          [&]<PrimitiveType Type>() {
            this->setFieldValue<typename PrimitiveTypeTraits<Type>::Representation>(field->offset(), 0);
          },
          [&](types::JStringRef) {
            this->setFieldValue<Instance*>(field->offset(), nullptr);
          },
          [&](const ArrayType&) {
            this->setFieldValue<Instance*>(field->offset(), nullptr);
          });
    }
  }
}

size_t Instance::getFieldOffset(types::JStringRef fieldName, types::JStringRef descriptor) const
{
  NameAndDescriptor key{fieldName, descriptor};
  assert(mClass->fields().contains(key));
  auto& field = mClass->fields().at(key);

  return field->offset();
}

std::unique_ptr<ArrayInstance> ArrayInstance::create(ArrayClass* arrayClass, size_t length)
{
  ArrayInstance* array = new (length) ArrayInstance(arrayClass, length);
  return std::unique_ptr<ArrayInstance>(array);
}

void* ArrayInstance::operator new(size_t base, size_t arrayLength)
{
  size_t size = base + arrayLength * sizeof(Value);
  return ::operator new(size);
}

ArrayInstance::ArrayInstance(ArrayClass* arrayClass, size_t length)
  : Instance(arrayClass), mLength(length)
{
  for (int32_t i = 0; i < mLength; i++) {
    this->setArrayElement(i, Value::defaultValue(arrayClass->fieldType()));
  }
}

ArrayInstance* Instance::asArrayInstance()
{
  assert(mClass->isArrayType());
  return static_cast<ArrayInstance*>(this);
}

ClassInstance* Instance::asClassInstance()
{
  assert(getClass()->className() == u"java/lang/Class");
  return static_cast<ClassInstance*>(this);
}
void* Instance::fieldsStart()
{
  return reinterpret_cast<char*>(this) + this->getClass()->headerSize();
}
const void* Instance::fieldsStart() const
{
  return reinterpret_cast<const char*>(this) + this->getClass()->headerSize();
}

JvmExpected<Value> ArrayInstance::getArrayElement(int32_t index)
{
  if (index < 0 || index >= mLength) {
    return makeError<Value>(u"java/lang/ArrayIndexOutOfBoundsException");
  }

  return *this->atIndex(index);
}

JvmExpected<void> ArrayInstance::setArrayElement(int32_t index, Value value)
{
  if (index < 0 || index >= mLength) {
    return makeError<void>(u"java/lang/ArrayIndexOutOfBoundsException");
  }

  *this->atIndex(index) = value;

  return JvmExpected<void>{};
}

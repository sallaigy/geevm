#include "vm/Instance.h"

using namespace geevm;

Instance::Instance(JClass* klass)
  : mClass(klass)
{
  mFields.resize(klass->fields().size(), Value::from<int32_t>(0));
  for (const auto& [key, field] : klass->fields()) {
    if (!hasAccessFlag(field->accessFlags(), FieldAccessFlags::ACC_STATIC)) {
      mFields[field->offset()] = Value::defaultValue(field->fieldType());
    }
  }
}

void Instance::setFieldValue(types::JStringRef fieldName, types::JStringRef descriptor, Value value)
{
  NameAndDescriptor key{fieldName, descriptor};
  assert(mClass->fields().contains(key));
  size_t offset = mClass->fields().at(key)->offset();

  mFields[offset] = value;
}

Value Instance::getFieldValue(types::JStringRef fieldName, types::JStringRef descriptor)
{
  NameAndDescriptor key{fieldName, descriptor};

  assert(mClass->fields().contains(key));
  size_t offset = mClass->fields().at(key)->offset();

  return mFields.at(offset);
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
  : Instance(arrayClass), mLength(length), mStart(this->contentsStart())
{
  for (int32_t i = 0; i < mLength; i++) {
    this->setArrayElement(i, Value::defaultValue(arrayClass->elementType()));
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

#include "vm/Instance.h"

using namespace geevm;

Instance::Instance(JClass* klass)
  : Instance(Kind::Object, klass)
{
}

Instance::Instance(Kind kind, JClass* klass)
  : mKind(kind), mClass(klass)
{
  for (const auto& [key, value] : klass->fields()) {
    if (!hasAccessFlag(value->accessFlags(), FieldAccessFlags::ACC_STATIC)) {
      mFields.emplace(key.first, Value::defaultValue(value->fieldType()));
    }
  }
}

void Instance::setFieldValue(types::JStringRef fieldName, Value value)
{
  assert(mFields.contains(fieldName));
  auto [it, success] = mFields.try_emplace(fieldName, value);
  if (!success) {
    it->second = value;
  }
}

Value Instance::getFieldValue(types::JStringRef fieldName)
{
  assert(mFields.contains(fieldName));
  return mFields.at(fieldName);
}

ArrayInstance::ArrayInstance(ArrayClass* arrayClass, size_t length)
  : Instance(Kind::Array, arrayClass), mContents(length, Value::defaultValue(arrayClass->elementType()))
{
}

ArrayInstance* Instance::asArrayInstance()
{
  assert(mKind == Kind::Array);
  return static_cast<ArrayInstance*>(this);
}

JvmExpected<Value> ArrayInstance::getArrayElement(int32_t index)
{
  if (index < 0 || index >= mContents.size()) {
    return makeError<Value, ArrayIndexOutOfBoundsException>();
  }

  return mContents[index];
}

JvmExpected<void> ArrayInstance::setArrayElement(int32_t index, Value value)
{
  if (index < 0 || index >= mContents.size()) {
    return makeError<void, ArrayIndexOutOfBoundsException>();
  }

  mContents[index] = value;

  return JvmExpected<void>{};
}

#include "vm/Instance.h"

using namespace geevm;

Instance::Instance(JClass* klass)
  : Instance(Kind::Object, klass)
{
}

Instance::Instance(Kind kind, JClass* klass)
  : mKind(kind), mClass(klass)
{
  mFields.resize(klass->fields().size(), Value::Int(0));
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

ArrayInstance::ArrayInstance(ArrayClass* arrayClass, size_t length)
  : Instance(Kind::Array, arrayClass), mContents(length, Value::defaultValue(arrayClass->elementType()))
{
}

ArrayInstance* Instance::asArrayInstance()
{
  assert(mKind == Kind::Array);
  return static_cast<ArrayInstance*>(this);
}

ClassInstance* Instance::asClassInstance()
{
  assert(getClass()->className() == u"java/lang/Class");
  return static_cast<ClassInstance*>(this);
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

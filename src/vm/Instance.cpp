#include "vm/Instance.h"

#include <iostream>

using namespace geevm;

Instance::Instance(JClass* klass)
  : mClass(klass)
{
  for (const auto& [key, field] : klass->fields()) {
    if (!field->isStatic()) {
      auto& fieldType = field->fieldType();
      if (auto primitiveType = fieldType.asPrimitive(); primitiveType) {
        switch (*primitiveType) {
          case PrimitiveType::Byte: this->setFieldValue<std::int8_t>(field->offset(), 0); break;
          case PrimitiveType::Char: this->setFieldValue<char16_t>(field->offset(), 0); break;
          case PrimitiveType::Double: this->setFieldValue<double>(field->offset(), 0); break;
          case PrimitiveType::Float: this->setFieldValue<float>(field->offset(), 0); break;
          case PrimitiveType::Int: this->setFieldValue<std::int32_t>(field->offset(), 0); break;
          case PrimitiveType::Long: this->setFieldValue<std::int64_t>(field->offset(), 0); break;
          case PrimitiveType::Short: this->setFieldValue<std::int16_t>(field->offset(), 0); break;
          case PrimitiveType::Boolean: this->setFieldValue<std::int32_t>(field->offset(), 0); break;
        }
      } else if (fieldType.asObjectName().has_value()) {
        this->setFieldValue<Instance*>(field->offset(), nullptr);
      }
    }
  }
}

void Instance::setFieldValue(types::JStringRef fieldName, types::JStringRef descriptor, Value value)
{
  NameAndDescriptor key{fieldName, descriptor};
  assert(mClass->fields().contains(key));
  auto& field = mClass->fields().at(key);

  // FIXME: Remove this once we no longer have Value
  auto& fieldType = field->fieldType();
  if (fieldType.asObjectName().has_value() || fieldType.dimensions() != 0) {
    this->setFieldValue<Instance*>(field->offset(), value.get<Instance*>());
  } else if (auto primitiveType = fieldType.asPrimitive(); primitiveType) {
    switch (*primitiveType) {
      case PrimitiveType::Byte: this->setFieldValue<std::int8_t>(field->offset(), value.get<int8_t>()); break;
      case PrimitiveType::Char: this->setFieldValue<char16_t>(field->offset(), value.get<char16_t>()); break;
      case PrimitiveType::Double: this->setFieldValue<double>(field->offset(), value.get<double>()); break;
      case PrimitiveType::Float: this->setFieldValue<float>(field->offset(), value.get<float>()); break;
      case PrimitiveType::Int: this->setFieldValue<std::int32_t>(field->offset(), value.get<int32_t>()); break;
      case PrimitiveType::Long: this->setFieldValue<std::int64_t>(field->offset(), value.get<int64_t>()); break;
      case PrimitiveType::Short: this->setFieldValue<std::int16_t>(field->offset(), value.get<int16_t>()); break;
      case PrimitiveType::Boolean: this->setFieldValue<std::int32_t>(field->offset(), value.get<int32_t>()); break;
    }
  }
}

Value Instance::getFieldValue(types::JStringRef fieldName, types::JStringRef descriptor)
{
  NameAndDescriptor key{fieldName, descriptor};

  assert(mClass->fields().contains(key));
  auto& field = mClass->fields().at(key);

  // FIXME: Remove this once we no longer have Value
  auto& fieldType = field->fieldType();
  if (fieldType.asObjectName().has_value() || fieldType.dimensions() != 0) {
    return Value::from<Instance*>(this->getFieldValue<Instance*>(field->offset()));
  } else if (auto primitiveType = fieldType.asPrimitive(); primitiveType) {
    switch (*primitiveType) {
      case PrimitiveType::Byte: return Value::from<std::int8_t>(this->getFieldValue<std::int8_t>(field->offset()));
      case PrimitiveType::Char: return Value::from<char16_t>(this->getFieldValue<char16_t>(field->offset()));
      case PrimitiveType::Double: return Value::from<double>(this->getFieldValue<double>(field->offset()));
      case PrimitiveType::Float: return Value::from<float>(this->getFieldValue<float>(field->offset()));
      case PrimitiveType::Int: return Value::from<std::int32_t>(this->getFieldValue<std::int32_t>(field->offset()));
      case PrimitiveType::Long: return Value::from<std::int64_t>(this->getFieldValue<std::int64_t>(field->offset()));
      case PrimitiveType::Short: return Value::from<std::int16_t>(this->getFieldValue<std::int16_t>(field->offset()));
      case PrimitiveType::Boolean: return Value::from<std::int32_t>(this->getFieldValue<std::int32_t>(field->offset()));
    }
  } else {
    std::unreachable();
  }
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

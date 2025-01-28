#include "vm/Instance.h"
#include "vm/Class.h"

#include <iostream>

using namespace geevm;

Instance::Instance(JClass* klass)
  : mClass(klass)
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
  assert(mClass->fields().contains(key));
  auto& field = mClass->fields().at(key);

  return field->offset();
}

int32_t Instance::hashCode()
{
  if (mHashCode == 0) {
    mHashCode = static_cast<int32_t>(std::hash<const Instance*>{}(this));
  }

  return mHashCode;
}

ArrayInstance::ArrayInstance(ArrayClass* arrayClass, size_t length)
  : Instance(arrayClass), mLength(length)
{
}

ArrayInstance* Instance::toArrayInstance()
{
  assert(mClass->isArrayType());
  return static_cast<ArrayInstance*>(this);
}

ClassInstance* Instance::toClassInstance()
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

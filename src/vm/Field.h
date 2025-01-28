#ifndef GEEVM_VM_FIELD_H
#define GEEVM_VM_FIELD_H

#include "class_file/Descriptor.h"

#include <utility>

namespace geevm
{

class InstanceClass;

class JField
{
public:
  JField(const FieldInfo& fieldInfo, InstanceClass* klass, types::JString name, types::JString descriptor, FieldType fieldType, size_t offset)
    : mFieldInfo(fieldInfo), mClass(klass), mFieldType(std::move(fieldType)), mName(std::move(name)), mDescriptor(descriptor), mOffset(offset)
  {
  }

  const FieldInfo& fieldInfo() const
  {
    return mFieldInfo;
  }

  InstanceClass* getClass() const
  {
    return mClass;
  }

  const types::JString& descriptor() const
  {
    return mDescriptor;
  }

  const FieldType& fieldType() const
  {
    return mFieldType;
  }

  FieldAccessFlags accessFlags() const
  {
    return mFieldInfo.accessFlags();
  }

  const types::JString& name() const
  {
    return mName;
  }

  /// Returns this field's offset within the given object/class.
  /// The value returned is contextual:
  ///  - for instance fields, it is the byte offset from the start of the object containing the field's value,
  ///  - for static fields, it is the index in the static fields vector that contains the slot for this field.
  size_t offset() const
  {
    return mOffset;
  }

  bool isStatic() const
  {
    return hasAccessFlag(mFieldInfo.accessFlags(), FieldAccessFlags::ACC_STATIC);
  }

  bool isFinal() const
  {
    return hasAccessFlag(mFieldInfo.accessFlags(), FieldAccessFlags::ACC_FINAL);
  }

  bool isPublic() const
  {
    return hasAccessFlag(mFieldInfo.accessFlags(), FieldAccessFlags::ACC_PUBLIC);
  }

  bool isProtected() const
  {
    return hasAccessFlag(mFieldInfo.accessFlags(), FieldAccessFlags::ACC_PROTECTED);
  }

  bool isPrivate() const
  {
    return hasAccessFlag(mFieldInfo.accessFlags(), FieldAccessFlags::ACC_PRIVATE);
  }

private:
  const FieldInfo& mFieldInfo;
  InstanceClass* mClass;
  FieldType mFieldType;
  types::JString mName;
  types::JString mDescriptor;
  size_t mOffset;
};

} // namespace geevm

#endif // GEEVM_VM_FIELD_H

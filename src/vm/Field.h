#ifndef GEEVM_VM_FIELD_H
#define GEEVM_VM_FIELD_H

#include <utility>

namespace geevm
{

struct FieldRef
{
  types::JString className;
  types::JString fieldName;
  types::JString fieldDescriptor;
};

class JField
{
public:
  JField(const FieldInfo& fieldInfo, FieldType fieldType)
    : mFieldInfo(fieldInfo), mFieldType(std::move(fieldType))
  {
  }

  const FieldInfo& fieldInfo() const
  {
    return mFieldInfo;
  }

  const FieldType& fieldType() const
  {
    return mFieldType;
  }

  FieldAccessFlags accessFlags() const
  {
    return mFieldInfo.accessFlags();
  }

  bool isStatic() const
  {
    return hasAccessFlag(mFieldInfo.accessFlags(), FieldAccessFlags::ACC_STATIC);
  }

  bool isFinal() const
  {
    return hasAccessFlag(mFieldInfo.accessFlags(), FieldAccessFlags::ACC_FINAL);
  }

private:
  const FieldInfo& mFieldInfo;
  FieldType mFieldType;
};

} // namespace geevm

#endif // GEEVM_VM_FIELD_H

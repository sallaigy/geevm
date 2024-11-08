#ifndef GEEVM_VM_FIELD_H
#define GEEVM_VM_FIELD_H

#include <utility>

namespace geevm
{

struct FieldRef
{
  JClass* klass;
  types::JString fieldName;
  types::JString fieldDescriptor;
};

class JField
{
public:
  JField(const FieldInfo& fieldInfo, types::JString name, FieldType fieldType)
    : mFieldInfo(fieldInfo), mName(std::move(name)), mFieldType(std::move(fieldType))
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

  const types::JString& name() const
  {
    return mName;
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

private:
  const FieldInfo& mFieldInfo;
  FieldType mFieldType;
  types::JString mName;
};

} // namespace geevm

#endif // GEEVM_VM_FIELD_H

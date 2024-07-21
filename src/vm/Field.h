#ifndef GEEVM_VM_FIELD_H
#define GEEVM_VM_FIELD_H

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
};

} // namespace geevm

#endif // GEEVM_VM_FIELD_H

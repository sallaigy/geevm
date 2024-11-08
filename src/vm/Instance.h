#ifndef GEEVM_VM_INSTANCE_H
#define GEEVM_VM_INSTANCE_H

#include "Class.h"
#include "common/JvmError.h"
#include "vm/Frame.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace geevm
{

class ArrayClass;
class AbstractClass;
class JClass;
class ArrayInstance;

class Instance
{
public:
  enum class Kind
  {
    Object,
    Array
  };

  explicit Instance(JClass* klass);

protected:
  Instance(Kind kind, JClass* klass);

public:
  JClass* getClass() const
  {
    return mClass;
  }

  void setFieldValue(types::JStringRef fieldName, Value value);
  Value getFieldValue(types::JStringRef fieldName);

  ArrayInstance* asArrayInstance();

  virtual ~Instance() = default;

protected:
  const Kind mKind;
  JClass* mClass;
  std::unordered_map<types::JStringRef, Value> mFields;
};

class ArrayInstance : public Instance
{
public:
  ArrayInstance(ArrayClass* arrayClass, size_t length);

  JvmExpected<Value> getArrayElement(int32_t index);
  JvmExpected<void> setArrayElement(int32_t index, Value value);

  int32_t length() const
  {
    return mContents.size();
  }

  const std::vector<Value>& contents() const
  {
    return mContents;
  }

private:
  std::vector<Value> mContents;
};

} // namespace geevm

#endif // GEEVM_VM_INSTANCE_H

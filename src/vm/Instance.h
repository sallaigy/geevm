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
class ClassInstance;

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

  void setFieldValue(types::JStringRef fieldName, types::JStringRef descriptor, Value value);
  Value getFieldValue(types::JStringRef fieldName, types::JStringRef descriptor);

  template<JvmType T>
  void setFieldValue(types::JStringRef fieldName, types::JStringRef descriptor, T value)
  {
    setFieldValue(fieldName, descriptor, Value::from<T>(value));
  }

  template<JvmType T>
  T getFieldValue(types::JStringRef fieldName, types::JStringRef descriptor)
  {
    return getFieldValue(fieldName, descriptor).get<T>();
  }

  ArrayInstance* asArrayInstance();
  ClassInstance* asClassInstance();

  virtual ~Instance() = default;

protected:
  const Kind mKind;
  JClass* mClass;
  std::vector<Value> mFields;
};

class ArrayInstance : public Instance
{
public:
  ArrayInstance(ArrayClass* arrayClass, size_t length);

  JvmExpected<Value> getArrayElement(int32_t index);
  JvmExpected<void> setArrayElement(int32_t index, Value value);

  template<JvmType T>
  JvmExpected<T> getArrayElement(int32_t index)
  {
    auto result = getArrayElement(index);

    return result.transform([](const Value& value) -> T { return value.get<T>(); });
  }

  template<JvmType T>
  JvmExpected<void> setArrayElement(int32_t index, T value)
  {
    return setArrayElement(index, Value::from<T>(value));
  }

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

/// An instance of java.lang.Class
class ClassInstance : public Instance
{
public:
  explicit ClassInstance(JClass* javaLangClass, JClass* target)
    : Instance(Kind::Object, javaLangClass), mTarget(target)
  {
  }

  JClass* target() const
  {
    return mTarget;
  }

private:
  JClass* mTarget;
};

} // namespace geevm

#endif // GEEVM_VM_INSTANCE_H

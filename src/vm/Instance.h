#ifndef GEEVM_VM_INSTANCE_H
#define GEEVM_VM_INSTANCE_H

#include "Class.h"
#include "common/JvmError.h"
#include "vm/Frame.h"

#include <cstdint>
#include <span>
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
  friend class JavaHeap;

protected:
  explicit Instance(JClass* klass);

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

  void* fieldsStart();
  const void* fieldsStart() const;

  virtual ~Instance() = default;

private:
  template<JvmType T>
  void setFieldValue(size_t offset, const T& value)
  {
    auto* ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(this) + offset);
    *ptr = value;
  }

  template<JvmType T>
  T getFieldValue(size_t offset)
  {
    auto* ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(this) + offset);
    return *ptr;
  }

protected:
  JClass* mClass;
};

class ArrayInstance : public Instance
{
public:
  ArrayInstance(ArrayClass* arrayClass, size_t length);

  static std::unique_ptr<ArrayInstance> create(ArrayClass* arrayClass, size_t length);

  void* operator new(size_t base, size_t arraySize);
  void operator delete(void* ptr)
  {
    ::operator delete(ptr);
  }

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
    return mLength;
  }

  using const_iterator = const Value*;
  const_iterator begin() const
  {
    return this->contentsStart();
  }
  const_iterator end() const
  {
    return this->contentsStart() + this->length();
  }

  std::span<Value> span()
  {
    return std::span<Value>(this->contentsStart(), static_cast<size_t>(this->length()));
  }

private:
  Value* contentsStart()
  {
    return reinterpret_cast<Value*>(this->fieldsStart());
  }

  const Value* contentsStart() const
  {
    return reinterpret_cast<const Value*>(this->fieldsStart());
  }

  Value* atIndex(size_t i)
  {
    return contentsStart() + i;
  }

private:
  int32_t mLength;
};

/// An instance of java.lang.Class
class ClassInstance : public Instance
{
  friend class JavaHeap;

  ClassInstance(JClass* javaLangClass, JClass* target)
    : Instance(javaLangClass), mTarget(target)
  {
  }

public:
  static std::unique_ptr<ClassInstance> create(JClass* javaLangClass, JClass* target);

  JClass* target() const
  {
    return mTarget;
  }

private:
  JClass* mTarget;
};

} // namespace geevm

#endif // GEEVM_VM_INSTANCE_H

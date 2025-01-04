#ifndef GEEVM_VM_INSTANCE_H
#define GEEVM_VM_INSTANCE_H

#include "Class.h"
#include "common/JvmError.h"
#include "vm/Frame.h"

#include <common/Memory.h>
#include <cstdint>
#include <span>

namespace geevm
{

class ArrayClass;
class AbstractClass;
class JClass;
class ArrayInstance;
class ClassInstance;

template<JvmType T>
class JavaArray;

class Instance
{
  friend class JavaHeap;
  friend class GarbageCollector;

protected:
  explicit Instance(JClass* klass);
  Instance(const Instance& other) = delete;

public:
  JClass* getClass() const
  {
    return mClass;
  }

  template<JvmType T>
  void setFieldValue(types::JStringRef fieldName, types::JStringRef descriptor, const T& value)
  {
    size_t offset = getFieldOffset(fieldName, descriptor);
    this->setFieldValue(offset, value);
  }

  template<JvmType T>
  T getFieldValue(types::JStringRef fieldName, types::JStringRef descriptor)
  {
    size_t offset = getFieldOffset(fieldName, descriptor);
    return this->getFieldValue<T>(offset);
  }

  template<JvmType T>
  T getFieldValue(size_t offset)
  {
    auto* ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(this) + offset);
    return *ptr;
  }

  ArrayInstance* asArrayInstance();

  template<JvmType T>
  JavaArray<T>* asArray()
  {
    // TODO: Assert class and type consistency
    return static_cast<JavaArray<T>*>(this);
  }

  ClassInstance* asClassInstance();

  void* fieldsStart();
  const void* fieldsStart() const;

  int32_t hashCode() const;

  Instance* copyTo(void* dest);

private:
  size_t getFieldOffset(types::JStringRef fieldName, types::JStringRef descriptor) const;

  template<JvmType T>
  void setFieldValue(size_t offset, const T& value)
  {
    auto* ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(this) + offset);
    *ptr = value;
  }

protected:
  JClass* mClass;
};

class ArrayInstance : public Instance
{
protected:
  ArrayInstance(ArrayClass* arrayClass, size_t length);

public:
  int32_t length() const
  {
    return mLength;
  }

private:
  int32_t mLength;
};

template<JvmType T>
class JavaArray : public ArrayInstance
{
public:
  JavaArray(ArrayClass* arrayClass, size_t length)
    : ArrayInstance(arrayClass, length)
  {
    for (int32_t i = 0; i < length; i++) {
      this->setArrayElement(i, static_cast<T>(0));
    }
  }

  JvmExpected<T> getArrayElement(int32_t index)
  {
    if (index < 0 || index >= length()) {
      return makeError<T>(u"java/lang/ArrayIndexOutOfBoundsException");
    }

    T result = *this->atIndex(index);
    return result;
  }

  JvmExpected<void> setArrayElement(int32_t index, T value)
  {
    if (index < 0 || index >= length()) {
      return makeError<void>(u"java/lang/ArrayIndexOutOfBoundsException");
    }

    *this->atIndex(index) = value;

    return JvmExpected<void>{};
  }

  using const_iterator = const T*;
  const_iterator begin() const
  {
    return reinterpret_cast<const T*>(this->fieldsStart());
  }
  const_iterator end() const
  {
    return reinterpret_cast<const T*>(this->fieldsStart()) + length();
  }

private:
  T* atIndex(size_t i)
  {
    return reinterpret_cast<T*>(this->fieldsStart()) + i;
  }
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

static_assert(std::is_trivially_copyable_v<Instance>);
static_assert(std::is_trivially_copyable_v<ArrayInstance>);
static_assert(std::is_trivially_copyable_v<ClassInstance>);

} // namespace geevm

#endif // GEEVM_VM_INSTANCE_H

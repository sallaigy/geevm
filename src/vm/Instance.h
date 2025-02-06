#ifndef GEEVM_VM_INSTANCE_H
#define GEEVM_VM_INSTANCE_H

#include "common/JvmError.h"
#include "vm/Value.h"

#include <cstdint>

namespace geevm
{

class ArrayClass;
class AbstractClass;
class JClass;
class ArrayInstance;
class ClassInstance;

template<JvmType T>
class JavaArray;

/// Instance of a java object.
///
/// This class stores all necessary information contained within an object, including the object's class, hash code,
/// and fields. Fields (or in the case of array instances, elements) are stored as trailing data allocated alongside
/// this object.
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

  ArrayInstance* toArrayInstance();
  ClassInstance* toClassInstance();

  template<JvmType T>
  JavaArray<T>* toArray()
  {
    // TODO: Assert class and type consistency
    return static_cast<JavaArray<T>*>(this);
  }

  int32_t hashCode();

private:
  size_t getFieldOffset(types::JStringRef fieldName, types::JStringRef descriptor) const;

  template<JvmType T>
  void setFieldValue(size_t offset, const T& value)
  {
    auto* ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(this) + offset);
    *ptr = value;
  }

private:
  JClass* mClass;

  /// Object hash code. As the garbage collector may relocate the object, we cannot reliably
  /// use the object address to produce the hash code. Instead, we use a lazily-calculated value
  /// derived from the object address at the time of the first call.
  int32_t mHashCode = 0;
};

/// Interface of a Java array.
///
/// Array elements are stored as trailing data allocated alongside instances of this class.
///
/// This class has no notion of the type stored, it merely knows the array length; use `JavaArray<T>` for setting and
/// getting individual elements.
class ArrayInstance : public Instance
{
protected:
  ArrayInstance(ArrayClass* arrayClass, size_t length);

public:
  int32_t length() const
  {
    return mLength;
  }

  /// Returns a pointer to the address where array elements start.
  constexpr void* elementsStart()
  {
    return reinterpret_cast<char*>(this) + sizeof(ArrayInstance);
  }

  constexpr const void* elementsStart() const
  {
    return reinterpret_cast<const char*>(this) + sizeof(ArrayInstance);
  }

private:
  int32_t mLength;
};

/// Strongly-typed handle for Java arrays.
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
    return reinterpret_cast<const T*>(this->elementsStart());
  }
  const_iterator end() const
  {
    return reinterpret_cast<const T*>(this->elementsStart()) + length();
  }

private:
  T* atIndex(size_t i)
  {
    return reinterpret_cast<T*>(this->elementsStart()) + i;
  }
};

/// An instance of 'java.lang.Class'
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

// As the garbage collector may relocate these types using memcpy, all object types _must_ be trivially copiable.
static_assert(std::is_trivially_copyable_v<Instance>);
static_assert(std::is_trivially_copyable_v<ArrayInstance>);
static_assert(std::is_trivially_copyable_v<ClassInstance>);
static_assert(std::is_trivially_destructible_v<Instance>);
static_assert(std::is_trivially_destructible_v<ArrayInstance>);
static_assert(std::is_trivially_destructible_v<ClassInstance>);

} // namespace geevm

#endif // GEEVM_VM_INSTANCE_H

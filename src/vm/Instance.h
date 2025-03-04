#ifndef GEEVM_VM_INSTANCE_H
#define GEEVM_VM_INSTANCE_H

#include "common/JvmError.h"

#include <cassert>
#include <cstdint>

namespace geevm
{

class ArrayClass;
class AbstractClass;
class JClass;
class InstanceClass;
class ArrayClass;

class ArrayInstance;
class ClassInstance;

template<JvmType T>
class JavaArray;

/// Instance and its subclasses cannot use inheritance as they need to be standard layout types in order to make
/// field offset calculations and assumptions about object layouts safe. This instance corresponds to the information
/// all instances of `java.lang.Object` needs.
struct InstanceHeader
{
  JClass* mClass = nullptr;
  int32_t mHashCode = 0;
};

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
  Instance() = default;

public:
  JClass* getClass() const
  {
    return reinterpret_cast<const InstanceHeader*>(this)->mClass;
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

protected:
  InstanceHeader& getHeader()
  {
    return *reinterpret_cast<InstanceHeader*>(this);
  }

  size_t getFieldOffset(types::JStringRef fieldName, types::JStringRef descriptor) const;

  template<JvmType T>
  void setFieldValue(size_t offset, const T& value)
  {
    auto* ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(this) + offset);
    *ptr = value;
  }
};

/// Standard Java object instance (as opposed to an array instance).
///
/// Note that this class is for any non-array Java objects that do not have dedicated instance classes declared for them.
/// Classes and strings have their own classes that do *not* inherit from this one.
class ObjectInstance : public Instance
{
public:
  explicit ObjectInstance(InstanceClass* klass);

private:
  InstanceHeader mHeader;
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
  ArrayInstance(ArrayClass* arrayClass, int32_t length);

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
  InstanceHeader mHeader;
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

  T& operator[](int32_t index)
  {
    assert(index >= 0 && index < length());
    return *this->atIndex(index);
  }

  const T& operator[](int32_t index) const
  {
    assert(index >= 0 && index < length());
    return *this->atIndex(index);
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
    : mHeader(javaLangClass), mTarget(target)
  {
  }

public:
  static std::unique_ptr<ClassInstance> create(JClass* javaLangClass, JClass* target);

  JClass* target() const
  {
    return mTarget;
  }

private:
  InstanceHeader mHeader;
  JClass* mTarget;
};

/// Instance of `java.lang.String`
class JavaString : public Instance
{
public:
  explicit JavaString(JClass* klass, JavaArray<int8_t>* value = nullptr, int8_t coder = 1)
    : mHeader(klass, 0), mValue(value), mCoder(coder)
  {
    this->verify();
  }

private:
  /// Verifies that the layout of this class definition is in sync with the memory layout expected from the JDK.
  /// The implementation of this method is disabled in non-debug builds.
  void verify();

private:
  InstanceHeader mHeader;
  // java.lang.String layout as defined in the JDK
  JavaArray<int8_t>* mValue;
  int8_t mCoder = 1;
  int32_t mHash = 0;
  bool mHashIsZero = false;
};

/// An instance of `java.lang.Throwable`
class JavaThrowable : public Instance
{
public:
  explicit JavaThrowable(JClass* klass)
    : mHeader(klass)
  {
    this->verify();
  }

private:
  void verify();

public:
  InstanceHeader mHeader;
  Instance* mBacktrace = nullptr;
  JavaString* mDetailMessage = nullptr;
  JavaThrowable* mCause = nullptr;
  JavaArray<Instance*>* mStackTrace = nullptr;
  int32_t mDepth = 0;
  Instance* mSuppressedExceptions = nullptr;
};

// Use static asserts to check that all object types are conforming to the assumptions we make about them.
// All object types MUST be:
//  - trivially copiable, as the garbage collector may relocate them using memcpy,
//  - trivially destructible, as the garbage collector frees the memory region of unreachable objects without calling any destructors,
//  - standard layout, as field accesses assume a certain class layout and some offsets are calculated using offsetof.
#define GEEVM_STATIC_CHECK_CLASS_PROPERTIES(NAME)    \
  static_assert(std::is_standard_layout_v<NAME>);    \
  static_assert(std::is_trivially_copyable_v<NAME>); \
  static_assert(std::is_trivially_destructible_v<NAME>);

GEEVM_STATIC_CHECK_CLASS_PROPERTIES(Instance);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(ArrayInstance);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(ClassInstance);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(JavaString);

GEEVM_STATIC_CHECK_CLASS_PROPERTIES(JavaArray<int8_t>);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(JavaArray<int16_t>);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(JavaArray<int32_t>);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(JavaArray<int64_t>);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(JavaArray<float>);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(JavaArray<double>);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(JavaArray<Instance*>);
GEEVM_STATIC_CHECK_CLASS_PROPERTIES(JavaArray<char16_t>);

#undef GEEVM_STATIC_CHECK_CLASS_PROPERTIES

} // namespace geevm

#endif // GEEVM_VM_INSTANCE_H

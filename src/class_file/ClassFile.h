#ifndef GEEVM_CLASS_FILE_CLASSFILE_H
#define GEEVM_CLASS_FILE_CLASSFILE_H

#include "Attributes.h"
#include "ConstantPool.h"

#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

namespace geevm
{

enum class ClassAccessFlags : types::u2
{
  ACC_PUBLIC = 0x0001,
  ACC_FINAL = 0x0010,
  ACC_SUPER = 0x0020,
  ACC_INTERFACE = 0x0200,
  ACC_ABSTRACT = 0x0400,
  ACC_SYNTHETIC = 0x1000,
  ACC_ANNOTATION = 0x2000,
  ACC_ENUM = 0x4000
};

enum class InnerClassAccessFlags : types::u2
{
  ACC_PUBLIC = 0x0001,
  ACC_PRIVATE = 0x0002,
  ACC_PROTECTED = 0x0004,
  ACC_STATIC = 0x0008,
  ACC_FINAL = 0x0010,
  ACC_INTERFACE = 0x0200,
  ACC_ABSTRACT = 0x0400,
  ACC_SYNTHETIC = 0x1000,
  ACC_ANNOTATION = 0x2000,
  ACC_ENUM = 0x4000
};

enum class FieldAccessFlags : types::u2
{
  ACC_PUBLIC = 0x0001,
  ACC_PRIVATE = 0x0002,
  ACC_PROTECTED = 0x0004,
  ACC_STATIC = 0x0008,
  ACC_FINAL = 0x0010,
  ACC_VOLATILE = 0x0040,
  ACC_TRANSIENT = 0x0080,
  ACC_SYNTHETIC = 0x1000,
  ACC_ENUM = 0x4000
};

enum class MethodAccessFlags : types::u2
{
  ACC_PUBLIC = 0x0001,
  ACC_PRIVATE = 0x0002,
  ACC_PROTECTED = 0x0004,
  ACC_STATIC = 0x0008,
  ACC_FINAL = 0x0010,
  ACC_SYNCHRONIZED = 0x0020,
  ACC_BRIDGE = 0x0040,
  ACC_VARARGS = 0x0080,
  ACC_NATIVE = 0x0100,
  ACC_ABSTRACT = 0x0400,
  ACC_STRICT = 0x0800,
  ACC_SYNTHETIC = 0x1000
};

enum class MethodParameterAccessFlags : types::u2
{
  ACC_FINAL = 0x0010,
  ACC_SYNTHETIC = 0x1000,
  ACC_MANDATED = 0x8000
};

template <class T>
concept AccessFlags = std::is_same_v<T, ClassAccessFlags> || std::is_same_v<T, InnerClassAccessFlags> || std::is_same_v<T, FieldAccessFlags> ||
                      std::is_same_v<T, MethodAccessFlags> || std::is_same_v<T, MethodParameterAccessFlags>;

template <AccessFlags T> constexpr T operator|(T lhs, T rhs)
{
  return static_cast<T>(static_cast<std::underlying_type_t<T>>(static_cast<types::u2>(lhs) | static_cast<std::underlying_type_t<T>>(rhs)));
}

template <AccessFlags T> constexpr T operator&(T lhs, T rhs)
{
  return static_cast<T>(static_cast<std::underlying_type_t<T>>(static_cast<types::u2>(lhs) & static_cast<std::underlying_type_t<T>>(rhs)));
}

class FieldInfo
{
public:
  FieldInfo(FieldAccessFlags accessFlags, types::u2 nameIndex, types::u2 descriptorIndex)
    : mAccessFlags(accessFlags), mNameIndex(nameIndex), mDescriptorIndex(descriptorIndex)
  {
  }

  FieldAccessFlags accessFlags() const
  {
    return mAccessFlags;
  }

  types::u2 nameIndex() const
  {
    return mNameIndex;
  }

  types::u2 descriptorIndex() const
  {
    return mDescriptorIndex;
  }

private:
  FieldAccessFlags mAccessFlags;
  types::u2 mNameIndex;
  types::u2 mDescriptorIndex;
};

class MethodInfo
{
public:
  MethodInfo(MethodAccessFlags accessFlags, types::u2 nameIndex, types::u2 descriptorIndex, std::optional<Code> code, std::vector<types::u2> exceptions)
    : mAccessFlags(accessFlags), mNameIndex(nameIndex), mDescriptorIndex(descriptorIndex), mCode(std::move(code)), mExceptions(std::move(exceptions))
  {
  }

  MethodInfo(const MethodInfo& other) = delete;
  MethodInfo(MethodInfo&& other) = default;

  MethodAccessFlags accessFlags() const
  {
    return mAccessFlags;
  }

  types::u2 nameIndex() const
  {
    return mNameIndex;
  }

  types::u2 descriptorIndex() const
  {
    return mDescriptorIndex;
  }

  bool hasCode() const
  {
    return mCode.has_value();
  }

  const Code& code() const
  {
    return *mCode;
  }

  const std::vector<types::u2>& exceptions() const
  {
    return mExceptions;
  }

private:
  MethodAccessFlags mAccessFlags;
  types::u2 mNameIndex;
  types::u2 mDescriptorIndex;
  std::optional<Code> mCode;
  std::vector<types::u2> mExceptions;
};

class ClassFile
{
public:
  static std::unique_ptr<ClassFile> fromFile(const std::string& filename);

  // Constructor
  //==--------------------------------------------------------------------==//

  ClassFile(types::u2 minorVersion, types::u2 majorVersion, std::unique_ptr<ConstantPool> constantPool, ClassAccessFlags accessFlags, types::u2 thisClass,
            types::u2 superClass, std::vector<types::u2> interfaces, std::vector<FieldInfo> fields, std::vector<MethodInfo> methods)
    : mMinorVersion(minorVersion),
      mMajorVersion(majorVersion),
      mConstantPool(std::move(constantPool)),
      mAccessFlags(accessFlags),
      mThisClass(thisClass),
      mSuperClass(superClass),
      mInterfaces(std::move(interfaces)),
      mFields(std::move(fields)),
      mMethods(std::move(methods))
  {
  }

  // Metadata
  //==--------------------------------------------------------------------==//
  types::u2 minorVersion() const
  {
    return mMinorVersion;
  }
  types::u2 majorVersion() const
  {
    return mMajorVersion;
  }
  ClassAccessFlags accessFlags() const
  {
    return mAccessFlags;
  }
  types::u2 thisClass() const
  {
    return mThisClass;
  }
  types::u2 superClass() const
  {
    return mSuperClass;
  }

  const std::vector<types::u2> interfaces() const
  {
    return mInterfaces;
  }
  const std::vector<FieldInfo>& fields() const
  {
    return mFields;
  }
  const std::vector<MethodInfo>& methods() const
  {
    return mMethods;
  }

  // Constant pool
  const ConstantPool& constantPool() const
  {
    return *mConstantPool;
  }

private:
  types::u2 mMinorVersion;
  types::u2 mMajorVersion;
  std::unique_ptr<ConstantPool> mConstantPool;
  ClassAccessFlags mAccessFlags;
  types::u2 mThisClass;
  types::u2 mSuperClass;

  std::vector<types::u2> mInterfaces;
  std::vector<FieldInfo> mFields;
  std::vector<MethodInfo> mMethods;

  // Attributes
  std::optional<types::u2> source_file_index;
};

class ClassFileReadError : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};

} // namespace geevm

#endif // GEEVM_CLASS_FILE_CLASSFILE_H

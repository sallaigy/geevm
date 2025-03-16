#ifndef GEEVM_CLASS_FILE_CONSTANTPOOL_H
#define GEEVM_CLASS_FILE_CONSTANTPOOL_H

#include "common/JvmTypes.h"

#include <cassert>
#include <optional>
#include <vector>

namespace geevm
{

class ConstantPool
{
public:
  enum class Tag : types::u1
  {
    Empty = 0,
    CONSTANT_Utf8 = 1,
    CONSTANT_Integer = 3,
    CONSTANT_Float = 4,
    CONSTANT_Long = 5,
    CONSTANT_Double = 6,
    CONSTANT_Class = 7,
    CONSTANT_String = 8,
    CONSTANT_Fieldref = 9,
    CONSTANT_Methodref = 10,
    CONSTANT_InterfaceMethodref = 11,
    CONSTANT_NameAndType = 12,
    CONSTANT_MethodHandle = 15,
    CONSTANT_MethodType = 16,
    CONSTANT_InvokeDynamic = 18,
  };

  struct Entry
  {
    Tag tag;
    union DataUnion
    {
      // Classes and interfaces (CONSTANT_Class_info) [4.4.1.]
      struct
      {
        types::u2 nameIndex;
      } classInfo;
      // Fields, methods and interface methods (CONSTANT_Fieldref_info, CONSTANT_Methodref_info,
      // CONSTANT_InterfaceMethodref_info) [4.4.2.]
      struct
      {
        types::u2 classIndex;
        types::u2 nameAndTypeIndex;
      } classAndNameRef;
      // References to strings (CONSTANT_String_info) [4.4.3.]
      struct
      {
        types::u2 stringIndex;
      } stringInfo;
      // Integer and float literals (CONSTANT_Integer_info, CONSTANT_Float_info) [4.4.4.]
      float singleFloat;
      int32_t singleInteger;
      // Long and double literals (CONSTANT_Long_info, CONSTANT_Double_info) [4.4.5.]
      double doubleFloat;
      int64_t doubleInteger;
      // Field or method with type (CONSTANT_NameAndType_info) [4.4.6.]
      struct
      {
        types::u2 nameIndex;
        types::u2 descriptorIndex;
      } nameAndType;
      // UTF-8 strings (CONSTANT_Utf8_info) [4.4.7.]
      types::u2 utf8String;
      // Method handles (CONSTANT_MethodHandle_info) [4.4.8.]
      struct
      {
        types::u1 referenceKind;
        types::u2 referenceIndex;
      } methodHandle;
      // Method types (CONSTANT_MethodType_info) [4.4.9.]
      struct
      {
        types::u2 descriptorIndex;
      } methodType;
      // `invokedynamic` information (CONSTANT_InvokeDynamic_info) [4.4.10.]
      struct
      {
        types::u2 bootstrapAddrIndex;
        types::u2 nameAndTypeIndex;
      } invokeDynamicInfo;
      // Empty entry to represent indices reserved by long and double literals
      struct
      {
      } empty;
    };

    static_assert(sizeof(DataUnion) == sizeof(uint64_t));
    static_assert(alignof(DataUnion) == alignof(uint64_t));

    DataUnion data;
  };

  // Constructors
  //==--------------------------------------------------------------------==//
  ConstantPool(std::vector<Entry> entries, std::vector<types::JString> strings)
    : mEntries(std::move(entries)), mStrings(std::move(strings))
  {
  }

  const Entry& getEntry(types::u2 index) const
  {
    assert(index > 0 && "Invalid constant pool index!");
    return mEntries[index - 1];
  }

  const Entry& operator[](types::u2 index) const
  {
    return this->getEntry(index);
  }

  types::JStringRef getString(types::u2 index) const;
  types::JStringRef getClassName(types::u2 index) const;
  std::optional<types::JStringRef> getOptionalClassName(types::u2 index) const;

  std::pair<types::JStringRef, types::JStringRef> getNameAndType(types::u2 index) const;

private:
  std::vector<Entry> mEntries;
  std::vector<types::JString> mStrings;
};

} // namespace geevm

#endif // GEEVM_CLASS_FILE_CONSTANTPOOL_H

#ifndef GEEVM_JVMTYPES_H
#define GEEVM_JVMTYPES_H

#include "common/TypeTraits.h"

#include <cstdint>
#include <string>

namespace geevm::types
{

using u1 = std::uint8_t;
using u2 = std::uint16_t;
using u4 = std::uint32_t;
using u8 = std::uint64_t;

using jchar = char16_t;

using JString = std::u16string;
using JStringRef = std::u16string_view;

std::string convertJString(const JString& str);
JString convertString(const std::string& str);

void replaceAll(JString& str, JStringRef oldValue, JStringRef newValue);

} // namespace geevm::types

namespace geevm
{

enum class PrimitiveType
{
  Byte,
  Char,
  Double,
  Float,
  Int,
  Long,
  Short,
  Boolean,
};

class JClass;
class Instance;

template<class T>
concept JvmType = is_one_of<T, std::int8_t, std::int16_t, std::int32_t, std::int64_t, char16_t, float, double, std::uint32_t, Instance*>();

template<class T>
concept CategoryTwoJvmType = is_one_of<T, std::int64_t, double>();

template<class T>
concept CategoryOneJvmType = JvmType<T> && !CategoryTwoJvmType<T>;

template<class T>
concept JvmPrimitiveType = is_one_of<std::int8_t, std::int16_t, std::int32_t, std::int64_t, char16_t, float, double, std::uint32_t>();

/// JVM types that are sign-extended to int before being pushed to the operand stack.
template<class T>
concept StoredAsInt = JvmType<T> && is_one_of<T, std::int8_t, std::int16_t, char16_t>();

template<class T>
concept JavaFloatType = is_one_of<T, float, double>();

template<class T>
concept JavaIntegerType = is_one_of<T, int8_t, int16_t, int32_t, int64_t>();

template<JvmType T>
struct JvmTypeTraits;

template<PrimitiveType Type>
struct PrimitiveTypeTraits;

template<>
struct PrimitiveTypeTraits<PrimitiveType::Byte>
{
  using Representation = std::int8_t;
  static constexpr types::JStringRef Descriptor = u"B";
  static constexpr types::JStringRef Name = u"byte";
  static constexpr types::JStringRef ClassName = u"java/lang/Byte";
};

template<>
struct PrimitiveTypeTraits<PrimitiveType::Char>
{
  using Representation = char16_t;
  static constexpr types::JStringRef Descriptor = u"C";
  static constexpr types::JStringRef Name = u"char";
  static constexpr types::JStringRef ClassName = u"java/lang/Char";
};

template<>
struct PrimitiveTypeTraits<PrimitiveType::Double>
{
  using Representation = double;
  static constexpr types::JStringRef Descriptor = u"D";
  static constexpr types::JStringRef Name = u"double";
  static constexpr types::JStringRef ClassName = u"java/lang/Double";
};

template<>
struct PrimitiveTypeTraits<PrimitiveType::Float>
{
  using Representation = float;
  static constexpr types::JStringRef Descriptor = u"F";
  static constexpr types::JStringRef Name = u"float";
  static constexpr types::JStringRef ClassName = u"java/lang/Float";
};

template<>
struct PrimitiveTypeTraits<PrimitiveType::Int>
{
  using Representation = std::int32_t;
  static constexpr types::JStringRef Descriptor = u"I";
  static constexpr types::JStringRef Name = u"int";
  static constexpr types::JStringRef ClassName = u"java/lang/Integer";
};

template<>
struct PrimitiveTypeTraits<PrimitiveType::Long>
{
  using Representation = std::int64_t;
  static constexpr types::JStringRef Descriptor = u"J";
  static constexpr types::JStringRef Name = u"long";
  static constexpr types::JStringRef ClassName = u"java/lang/Long";
};

template<>
struct PrimitiveTypeTraits<PrimitiveType::Short>
{
  using Representation = std::int16_t;
  static constexpr types::JStringRef Descriptor = u"S";
  static constexpr types::JStringRef Name = u"short";
  static constexpr types::JStringRef ClassName = u"java/lang/Short";
};

template<>
struct PrimitiveTypeTraits<PrimitiveType::Boolean>
{
  using Representation = std::int32_t;
  static constexpr types::JStringRef Descriptor = u"Z";
  static constexpr types::JStringRef Name = u"boolean";
  static constexpr types::JStringRef ClassName = u"java/lang/Boolean";
};

using NameAndDescriptor = std::pair<types::JString, types::JString>;

struct ClassNameAndDescriptor
{
  // Hash code functor
  struct Hash
  {
    std::size_t operator()(const ClassNameAndDescriptor& name) const
    {
      std::size_t hash = 17;
      hash = hash * 31 + std::hash<types::JString>()(name.className);
      hash = hash * 31 + std::hash<types::JString>()(name.name);
      return hash * 31 + std::hash<types::JString>()(name.descriptor);
    }
  };

  // Fields
  types::JString className;
  types::JString name;
  types::JString descriptor;

  bool operator==(const ClassNameAndDescriptor&) const = default;
};

} // namespace geevm

#endif // GEEVM_JVMTYPES_H

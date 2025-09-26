#ifndef GEEVM_JVMTYPES_H
#define GEEVM_JVMTYPES_H

#include "common/Debug.h"
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

void replaceAll(JString& str, JStringRef oldValue, JStringRef newValue);

} // namespace geevm::types

namespace geevm
{

enum class PrimitiveType
{
  Byte = 8,
  Char = 5,
  Double = 7,
  Float = 6,
  Int = 10,
  Long = 11,
  Short = 9,
  Boolean = 4,
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

#define PRIMITIVE_TYPE_DEFINITION(TYPE, REPR, DESCRIPTOR, NAME, CLASSNAME) \
  template<>                                                               \
  struct PrimitiveTypeTraits<PrimitiveType::TYPE>                          \
  {                                                                        \
    using Representation = REPR;                                           \
    static constexpr types::JStringRef Descriptor = DESCRIPTOR;            \
    static constexpr types::JStringRef Name = NAME;                        \
    static constexpr types::JStringRef ClassName = CLASSNAME;              \
    static constexpr types::JStringRef ArrayClassName = u"[" DESCRIPTOR;   \
  };

PRIMITIVE_TYPE_DEFINITION(Byte, std::int8_t, u"B", u"byte", u"java/lang/Byte");
PRIMITIVE_TYPE_DEFINITION(Char, char16_t, u"C", u"char", u"java/lang/Char");
PRIMITIVE_TYPE_DEFINITION(Double, double, u"D", u"double", u"java/lang/Double");
PRIMITIVE_TYPE_DEFINITION(Float, float, u"F", u"float", u"java/lang/Float");
PRIMITIVE_TYPE_DEFINITION(Int, std::int32_t, u"I", u"int", u"java/lang/Int");
PRIMITIVE_TYPE_DEFINITION(Long, std::int64_t, u"J", u"long", u"java/lang/Long");
PRIMITIVE_TYPE_DEFINITION(Short, std::int16_t, u"S", u"short", u"java/lang/Short");
PRIMITIVE_TYPE_DEFINITION(Boolean, std::int8_t, u"Z", u"boolean", u"java/lang/Boolean");

template<class MapFunc>
decltype(auto) mapPrimitive(PrimitiveType type, const MapFunc& mapper)
{
  switch (type) {
    case PrimitiveType::Byte: return mapper.template operator()<PrimitiveType::Byte>();
    case PrimitiveType::Char: return mapper.template operator()<PrimitiveType::Char>();
    case PrimitiveType::Double: return mapper.template operator()<PrimitiveType::Double>();
    case PrimitiveType::Float: return mapper.template operator()<PrimitiveType::Float>();
    case PrimitiveType::Int: return mapper.template operator()<PrimitiveType::Int>();
    case PrimitiveType::Long: return mapper.template operator()<PrimitiveType::Long>();
    case PrimitiveType::Short: return mapper.template operator()<PrimitiveType::Short>();
    case PrimitiveType::Boolean: return mapper.template operator()<PrimitiveType::Boolean>();
  }
  GEEVM_UNREACHBLE("Unknown primitive type");
}

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

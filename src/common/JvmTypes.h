#ifndef GEEVM_JVMTYPES_H
#define GEEVM_JVMTYPES_H

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

} // namespace geevm::types

namespace geevm
{

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

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

using JString = std::u16string;
using JStringRef = std::u16string_view;

std::string convertJString(const JString& str);
JString convertString(const std::string& str);

} // namespace geevm::types

namespace geevm
{

using NameAndDescriptor = std::pair<types::JString, types::JString>;

} // namespace geevm

#endif // GEEVM_JVMTYPES_H

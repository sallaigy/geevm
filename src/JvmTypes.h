#ifndef GEEVM_JVMTYPES_H
#define GEEVM_JVMTYPES_H

#include <cstdint>
#include <string>

namespace geevm::types
{

using u1  = std::uint8_t;
using u2 = std::uint16_t;
using u4 = std::uint32_t;
using u8 = std::uint64_t;

using JString = std::u16string;

std::string convertJString(const JString& str);

} // namespace geevm::types

#endif //GEEVM_JVMTYPES_H

#ifndef GEEVM_COMMON_ENCODING_H
#define GEEVM_COMMON_ENCODING_H

#include "common/JvmTypes.h"

#include <span>
#include <string>

namespace geevm
{

/// Translates a sequence of bytes encoded in JVM's modified UTF-8 to UTF-8.
///
/// The difference between modified UTF-8 and standard UTF-8 is:
///   - The null code point '\u0000' is encoded in 2-byte format.
///   - Only the 1-byte, 2-byte, and 3-byte formats are used.
///   - Supplementary characters are represented in the form of surrogate pairs.
std::string decodeJvmUtf8(std::span<uint8_t> bytes);

std::u16string utf8ToUtf16(std::string_view utf8);

std::string utf16ToUtf8(std::u16string_view utf16);

} // namespace geevm

#endif // GEEVM_COMMON_ENCODING_H

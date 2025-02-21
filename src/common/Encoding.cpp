#include "common/Encoding.h"
#include "common/JvmError.h"

#include <codecvt>
#include <locale>
#include <vector>

using namespace geevm;

std::string geevm::decodeJvmUtf8(std::span<uint8_t> bytes)
{
  types::u2 i = 0;

  std::string buffer;
  while (i < bytes.size()) {
    if ((bytes[i] & 0b1'0000000) == 0) {
      // 1) '\u0001' to '\u007F' is represented by a single byte. If the byte begins with a zero bit,
      //  we are in this case.
      buffer.push_back(std::bit_cast<char>(bytes[i]));
      i += 1;
    } else if ((bytes[i] & 0b111'00000) == 0b110'00000 && (bytes[i + 1] & 0b11'000000) == 0b10'000000) {
      // 2) '\u0000' and from '\u0080' to '\u07FF' is two bytes:
      uint16_t codePoint = ((bytes[i] & 0x1F) << 6) + (bytes[i + 1] & 0x3F);
      if (codePoint == 0) {
        buffer.push_back(0);
      } else {
        buffer.push_back(std::bit_cast<char>(bytes[i]));
        buffer.push_back(std::bit_cast<char>(bytes[i + 1]));
      }
      i += 2;
    } else if ((bytes[i] & 0b1111'0000) == 0b1110'0000 && (bytes[i + 1] & 0b11'000000) == 0b10'000000 && (bytes[i + 2] & 0b11'000000) == 0b10'000000) {
      if (bytes[i] != 0b11101101 || (bytes[i + 1] & 0b1111'0000) != 0b1010'0000 || (bytes[i + 2] & 0b10'000000) != 0b10'000000 || bytes[i + 3] != 0b11101101 ||
          (bytes[i + 4] & 0b1111'0000) != 0b1011'0000 || (bytes[i + 5] & 0b11'000000) != 0b10'000000) {
        // 3) '\u0800' to  '\uFFFF' is three bytes,
        buffer.push_back(std::bit_cast<char>(bytes[i]));
        buffer.push_back(std::bit_cast<char>(bytes[i + 1]));
        buffer.push_back(std::bit_cast<char>(bytes[i + 2]));
        i += 3;
      } else {
        // 4) everything above '\uFFFF' is six bytes.
        uint32_t codePoint = 0x10000 + ((bytes[i + 1] & 0x0f) << 16) + ((bytes[i + 2] & 0x3f) << 10) + ((bytes[i + 4] & 0x0f) << 6) + (bytes[i + 5] & 0x3f);

        buffer.push_back(static_cast<char>(0xf0 | codePoint >> 18));
        buffer.push_back(static_cast<char>(0x80 | codePoint >> 12 & 0x3F));
        buffer.push_back(static_cast<char>(0x80 | codePoint >> 6 & 0x3F));
        buffer.push_back(static_cast<char>(0x80 | codePoint & 0x3F));
        i += 6;
      }
    } else {
      geevm_panic("Invalid byte character in class file!");
    }
  }

  return buffer;
}

std::u16string geevm::utf8ToUtf16(std::string_view utf8)
{
  // FIXME: codecvt is deprecated
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
  return converter.from_bytes(utf8.data());
}

std::string geevm::utf16ToUtf8(std::u16string_view utf16)
{
  // FIXME: codecvt is deprecated
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
  return converter.to_bytes(utf16.data());
}

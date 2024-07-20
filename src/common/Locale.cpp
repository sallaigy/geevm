#include "JvmTypes.h"

#include <codecvt>
#include <locale>

std::string geevm::types::convertJString(const geevm::types::JString& str)
{
  // Convert u16string to std::string
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
  return converter.to_bytes(str);
}

geevm::types::JString geevm::types::convertString(const std::string& str)
{
  // Convert std::string to u16string
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
  return converter.from_bytes(str);
}

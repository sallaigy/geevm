#include "JvmTypes.h"

#include <locale>
#include <codecvt>

std::string geevm::types::convertJString(const geevm::types::JString& str)
{
  // Convert u16string to std::string
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
  return converter.to_bytes(str);
}

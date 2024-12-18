#include "common/JvmTypes.h"

using namespace geevm;

void types::replaceAll(JString& str, JStringRef oldValue, JStringRef newValue)
{
  size_t pos = 0;
  while ((pos = str.find(oldValue, pos)) != JString::npos) {
    str.replace(pos, oldValue.length(), newValue);
    pos += oldValue.length();
  }
}

#include "vm/VmUtils.h"
#include "vm/Instance.h"

#include <algorithm>

using namespace geevm;

types::JString utils::getStringValue(Instance* stringInstance)
{
  assert(stringInstance->getClass()->className() == u"java/lang/String");

  ArrayInstance* array = stringInstance->getFieldValue<Instance*>(u"value", u"[B")->asArrayInstance();

  types::JString result;
  for (int i = 0; i < array->length(); i += 2) {
    auto hi = array->getArrayElement(i)->get<int8_t>();
    auto lo = array->getArrayElement(i + 1)->get<int8_t>();

    uint16_t value = std::bit_cast<uint16_t>(static_cast<int16_t>(hi)) | (std::bit_cast<uint16_t>(static_cast<int16_t>(lo)) << 8);

    result += std::bit_cast<char16_t>(value);
  }

  return result;
}

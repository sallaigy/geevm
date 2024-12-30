#include "vm/VmUtils.h"
#include "vm/Instance.h"

#include <algorithm>

using namespace geevm;

types::JString utils::getStringValue(Instance* stringInstance)
{
  assert(stringInstance->getClass()->className() == u"java/lang/String");

  JavaArray<int8_t>* array = stringInstance->getFieldValue<Instance*>(u"value", u"[B")->asArray<int8_t>();

  types::JString result;
  for (int i = 0; i < array->length(); i += 2) {
    int8_t hi = *array->getArrayElement(i);
    int8_t lo = *array->getArrayElement(i + 1);

    uint16_t value = std::bit_cast<uint16_t>(static_cast<int16_t>(hi)) | (std::bit_cast<uint16_t>(static_cast<int16_t>(lo)) << 8);

    result += std::bit_cast<char16_t>(value);
  }

  return result;
}

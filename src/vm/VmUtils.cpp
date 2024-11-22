#include "vm/VmUtils.h"
#include "vm/Instance.h"

#include <algorithm>

using namespace geevm;

types::JString utils::getStringValue(Instance* stringInstance)
{
  assert(stringInstance->getClass()->className() == u"java/lang/String");

  ArrayInstance* array = stringInstance->getFieldValue<Instance*>(u"value", u"[C")->asArrayInstance();

  types::JString result;
  for (Value ch : array->contents()) {
    result += ch.get<char16_t>();
  }

  return result;
}

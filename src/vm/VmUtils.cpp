#include "vm/VmUtils.h"
#include "vm/Instance.h"

#include <algorithm>

using namespace geevm;

types::JString utils::getStringValue(Instance* stringInstance)
{
  assert(stringInstance->getClass()->className() == u"java/lang/String");

  ArrayInstance* array = stringInstance->getFieldValue(u"value", u"[C").asReference()->asArrayInstance();

  types::JString result;
  for (Value ch : array->contents()) {
    result += ch.asChar();
  }

  return result;
}

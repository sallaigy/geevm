#include "vm/StringHeap.h"
#include "vm/Vm.h"

using namespace geevm;

Instance* StringHeap::intern(types::JStringRef utf8)
{
  if (auto it = mInternedStrings.find(utf8); it != mInternedStrings.end()) {
    return it->second;
  }

  auto stringClass = mVm.resolveClass(u"java/lang/String");
  assert(stringClass.has_value() && "java/lang/String should be available!");

  auto charArrayClass = mVm.resolveClass(u"[C");
  assert(charArrayClass.has_value() && "The array class '[C' should be available!");

  ArrayInstance* stringContents = mVm.heap().allocateArray((*charArrayClass)->asArrayClass(), utf8.size());
  for (int32_t i = 0; i < utf8.size(); ++i) {
    stringContents->setArrayElement(i, Value::Char(utf8[i]));
  }

  Instance* newInstance = mVm.heap().allocate((*stringClass)->asInstanceClass());
  newInstance->setFieldValue(u"value", u"[C", Value::Reference(stringContents));

  auto [res, _] = mInternedStrings.try_emplace(utf8, newInstance);
  return res->second;
}

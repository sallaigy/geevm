#include "vm/StringHeap.h"
#include "vm/Vm.h"

using namespace geevm;

Instance* StringHeap::intern(types::JStringRef utf8)
{
  if (auto it = mInternedStrings.find(utf8); it != mInternedStrings.end()) {
    return it->second;
  }

  auto stringClass = mVm.resolveClass(u"java/lang/String");
  if (!stringClass) {
    mVm.raiseError(*stringClass.error());
  }

  auto charArrayClass = mVm.resolveClass(u"[C");
  if (!charArrayClass) {
    mVm.raiseError(*charArrayClass.error());
  }

  ArrayInstance* stringContents = mVm.newArrayInstance((*charArrayClass)->asArrayClass(), utf8.size());
  for (int32_t i = 0; i < utf8.size(); ++i) {
    stringContents->setArrayElement(i, Value::Char(utf8[i]));
  }

  Instance* newInstance = mVm.newInstance((*stringClass)->asInstanceClass());
  newInstance->setFieldValue(u"value", Value::Reference(stringContents));

  auto [res, _] = mInternedStrings.try_emplace(utf8, newInstance);
  return res->second;
}

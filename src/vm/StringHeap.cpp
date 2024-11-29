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

  auto byteArrayClass = mVm.resolveClass(u"[B");
  assert(byteArrayClass.has_value() && "The array class '[B' should be available!");

  std::vector<uint8_t> bytes;
  for (char16_t c : utf8) {
    bytes.push_back(c & 0xff);
    bytes.push_back((c >> 8) & 0xff);
  }

  ArrayInstance* stringContents = mVm.heap().allocateArray((*byteArrayClass)->asArrayClass(), bytes.size());
  for (int32_t i = 0; i < bytes.size(); ++i) {
    stringContents->setArrayElement<int8_t>(i, std::bit_cast<int8_t>(bytes[i]));
  }

  Instance* newInstance = mVm.heap().allocate((*stringClass)->asInstanceClass());
  newInstance->setFieldValue<Instance*>(u"value", u"[B", stringContents);
  // newInstance->setFieldValue<int8_t>(u"coder", u"B", Value::from<int8_t>(-1));

  auto [res, _] = mInternedStrings.try_emplace(utf8, newInstance);
  return res->second;
}

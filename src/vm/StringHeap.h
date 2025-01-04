#ifndef GEEVM_STRINGHEAP_H
#define GEEVM_STRINGHEAP_H

#include "GarbageCollector.h"

#include <common/JvmTypes.h>
#include <unordered_map>

namespace geevm
{

class Vm;
class Instance;

class StringHeap
{
public:
  explicit StringHeap(Vm& vm);

  Instance* intern(const types::JString& utf8);
  std::unordered_map<types::JString, Instance*>& strings()
  {
    return mInternedStrings;
  }

private:
  Vm& mVm;
  std::unordered_map<types::JString, Instance*> mInternedStrings;
};

} // namespace geevm

#endif // GEEVM_STRINGHEAP_H

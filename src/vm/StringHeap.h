#ifndef GEEVM_STRINGHEAP_H
#define GEEVM_STRINGHEAP_H

#include "common/JvmTypes.h"
#include "vm/GarbageCollector.h"

#include <unordered_map>

namespace geevm
{

class Vm;
class Instance;

class StringHeap
{
public:
  explicit StringHeap(Vm& vm);

  GcRootRef<Instance> intern(const types::JString& utf8);
  std::unordered_map<types::JString, GcRootRef<>>& strings()
  {
    return mInternedStrings;
  }

private:
  Vm& mVm;
  std::unordered_map<types::JString, GcRootRef<>> mInternedStrings;
};

} // namespace geevm

#endif // GEEVM_STRINGHEAP_H

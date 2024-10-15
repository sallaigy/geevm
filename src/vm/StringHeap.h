#ifndef GEEVM_STRINGHEAP_H
#define GEEVM_STRINGHEAP_H
#include "Instance.h"

#include <common/JvmTypes.h>
#include <unordered_map>
#include <vector>

namespace geevm
{

class Vm;

class StringHeap
{
public:
  explicit StringHeap(Vm& vm)
    : mVm(vm)
  {
  }

  int32_t hash(types::JStringRef utf8) const;

  Instance* intern(types::JStringRef utf8);

private:
  Vm& mVm;
  std::unordered_map<types::JStringRef, Instance*> mInternedStrings;
};

} // namespace geevm

#endif // GEEVM_STRINGHEAP_H

#ifndef GEEVM_STRINGHEAP_H
#define GEEVM_STRINGHEAP_H

#include <common/JvmTypes.h>
#include <unordered_map>

namespace geevm
{

class Vm;
class Instance;

class StringHeap
{
public:
  explicit StringHeap(Vm& vm)
    : mVm(vm)
  {
  }

  Instance* intern(const types::JString& utf8);

private:
  Vm& mVm;
  std::unordered_map<types::JString, Instance*> mInternedStrings;
};

} // namespace geevm

#endif // GEEVM_STRINGHEAP_H

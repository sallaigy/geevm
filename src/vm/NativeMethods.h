#ifndef GEEVM_NATIVEMETHODS_H
#define GEEVM_NATIVEMETHODS_H

#include "common/Hash.h"
#include "vm/Frame.h"
#include "vm/Vm.h"

#include <functional>

namespace geevm
{

using NativeMethodHandle = std::function<std::optional<Value>(Vm& vm, CallFrame& frame, const std::vector<Value>& args)>;

class NativeMethodRegistry
{

public:
private:
  std::unordered_map<NameAndDescriptor, NativeMethodHandle, PairHash> mNativeMethods;
};

} // namespace geevm

#endif // GEEVM_NATIVEMETHODS_H

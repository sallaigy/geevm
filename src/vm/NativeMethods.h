#ifndef GEEVM_NATIVEMETHODS_H
#define GEEVM_NATIVEMETHODS_H

#include "vm/Frame.h"

#include <functional>

namespace geevm
{

class Vm;
class CallFrame;

using NativeMethodHandle = std::function<std::optional<Value>(Vm& vm, CallFrame& frame, const std::vector<Value>& args)>;

class NativeMethodRegistry
{
public:
  void registerNativeMethod(const ClassNameAndDescriptor& method, NativeMethodHandle handle);
  std::optional<NativeMethodHandle> get(const ClassNameAndDescriptor& method) const;

private:
  std::unordered_map<ClassNameAndDescriptor, NativeMethodHandle, ClassNameAndDescriptor::Hash> mNativeMethods;
};

} // namespace geevm

#endif // GEEVM_NATIVEMETHODS_H

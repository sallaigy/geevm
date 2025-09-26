#ifndef GEEVM_VM_JIT_JITCOMPILER_H
#define GEEVM_VM_JIT_JITCOMPILER_H

#include "vm/Method.h"

namespace geevm
{

class Vm;

class JitCompiler
{
public:
  /// Creates a platform-specific JIT compiler
  static std::unique_ptr<JitCompiler> create(Vm& vm);

  /// Performs JIT compilation on the given method.
  virtual JitFunction compile(JMethod* method) = 0;

  virtual ~JitCompiler() = default;
};

} // namespace geevm

#endif // GEEVM_VM_JIT_JITCOMPILER_H

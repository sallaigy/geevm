#ifndef GEEVM_VM_GCROOTS_H
#define GEEVM_VM_GCROOTS_H

#include "common/JvmTypes.h"

#include <generator>
#include <vector>

namespace geevm
{

class Instance;
class JavaThread;
class JMethod;
class CallFrame;

class FrameRoots
{
  FrameRoots(std::vector<bool> localVariableReferences, std::vector<bool> operandStackReferences)
    : mLocalVariableReferences(localVariableReferences), mOperandStackReferences(operandStackReferences)
  {
  }

public:
  static FrameRoots compute(JMethod* method, types::u4 pos);

  std::generator<std::pair<uint16_t, Instance*>> referencesInLocals(CallFrame& frame) const;
  std::generator<std::pair<uint16_t, Instance*>> referencesInOperandStack(CallFrame& frame) const;

private:
  std::vector<bool> mLocalVariableReferences;
  std::vector<bool> mOperandStackReferences;
};

/// Collects all live references on the call stack of the given Java thread.
std::generator<std::pair<uint16_t, Instance*>> collectCallStackReferences(JavaThread& thread);

} // namespace geevm

#endif // GEEVM_VM_GCROOTS_H

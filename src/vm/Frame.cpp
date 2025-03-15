#include "vm/Frame.h"

using namespace geevm;

CallFrame::CallFrame(JMethod* method, CallFrame* previous, std::uint64_t* localVariables, bool* localVariableReferences, std::uint64_t* operandStack,
                     bool* operandStackReferences)
  : mMethod(method),
    mPrevious(previous),
    mLocalVariables(localVariables),
    mLocalVariableReferences(localVariableReferences),
    mOperandStack(operandStack),
    mOperandStackReferences(operandStackReferences)
{
  if (!method->isNative()) {
    mCode = mMethod->getCode().bytes().data();

    uint16_t maxLocals = method->getCode().maxLocals();
    uint16_t maxStack = mMethod->getCode().maxStack();

    std::memset(mLocalVariables, 0, sizeof(uint64_t) * maxLocals);
    std::memset(mLocalVariableReferences, 0, sizeof(bool) * maxLocals);
    std::memset(mOperandStack, 0, sizeof(uint64_t) * maxStack);
    std::memset(mOperandStackReferences, 0, sizeof(bool) * maxStack);
  }
}

CallFrame::~CallFrame()
{
}

void CallFrame::prepareCall(CallFrame& other, uint16_t numArgs)
{
  int32_t startIndex = mOperandStackPointer - numArgs;
  assert(startIndex >= 0);
  std::memcpy(other.mLocalVariables, mOperandStack + startIndex, numArgs * sizeof(uint64_t));
  std::memcpy(other.mLocalVariableReferences, mOperandStackReferences + startIndex, numArgs * sizeof(bool));
  // The operand stack is not popped until the call is actually finished
}

#include "vm/Frame.h"

#include <cstring>

using namespace geevm;

CallFrame::CallFrame(JMethod* method, CallFrame* previous, std::uint64_t* localVariables, std::uint64_t* operandStack)
  : mMethod(method), mPrevious(previous), mLocalVariables(localVariables), mOperandStack(operandStack)
{
  if (!method->isNative()) {
    mCode = mMethod->getCode().bytes().data();

    uint16_t maxLocals = method->getCode().maxLocals();
    uint16_t maxStack = mMethod->getCode().maxStack();

    std::memset(mLocalVariables, 0, sizeof(uint64_t) * maxLocals);
    std::memset(mOperandStack, 0, sizeof(uint64_t) * maxStack);
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
  // The operand stack is not popped until the call is actually finished
}

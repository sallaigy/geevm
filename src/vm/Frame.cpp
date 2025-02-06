#include "vm/Frame.h"

using namespace geevm;

CallFrame::CallFrame(JMethod* method, CallFrame* previous)
  : mMethod(method), mPrevious(previous)
{
  if (!method->isNative()) {
    mCode = mMethod->getCode().bytes().data();

    uint16_t maxLocals = method->getCode().maxLocals();
    uint16_t maxStack = mMethod->getCode().maxStack();

    mLocalVariables = new uint64_t[maxLocals];
    mLocalVariableReferences = new bool[maxLocals];
    mOperandStack = new uint64_t[maxStack];
    mOperandStackReferences = new bool[maxStack];
    mOperandStackPointer = 0;

    std::memset(mLocalVariables, 0, sizeof(uint64_t) * maxLocals);
    std::memset(mLocalVariableReferences, 0, sizeof(bool) * maxLocals);
    std::memset(mOperandStack, 0, sizeof(uint64_t) * maxStack);
    std::memset(mOperandStackReferences, 0, sizeof(bool) * maxStack);
  }
}

CallFrame::~CallFrame()
{
  // Even if the frame was native, deleting nullptr is still valid
  delete[] mLocalVariables;
  delete[] mLocalVariableReferences;
  delete[] mOperandStack;
  delete[] mOperandStackReferences;
}

void CallFrame::prepareCall(CallFrame& other, uint16_t numArgs)
{
  int32_t startIndex = mOperandStackPointer - numArgs;
  assert(startIndex >= 0);
  std::memcpy(other.mLocalVariables, mOperandStack + startIndex, numArgs * sizeof(uint64_t));
  std::memcpy(other.mLocalVariableReferences, mOperandStackReferences + startIndex, numArgs * sizeof(bool));
  mOperandStackPointer = startIndex;
}

#ifndef GEEVM_VM_FRAME_H
#define GEEVM_VM_FRAME_H

#include "vm/GarbageCollector.h"
#include "vm/Method.h"
#include "vm/Value.h"

#include <cassert>

namespace geevm
{
class InstanceClass;

class CallFrame
{
public:
  explicit CallFrame(JMethod* method, CallFrame* previous)
    : mMethod(method), mPrevious(previous)
  {
    if (!method->isNative()) {
      mLocalVariables.assign(method->getCode().maxLocals(), Value::from<int32_t>(0));
      mOperandStack.reserve(method->getCode().maxStack());
    }
  }

  InstanceClass* currentClass() const
  {
    return mMethod->getClass();
  }

  JMethod* currentMethod() const
  {
    return mMethod;
  }

  // Local variable manipulation
  //==--------------------------------------------------------------------==//

  template<CategoryOneJvmType T>
  void storeValue(types::u2 index, T value)
  {
    assert(index < mLocalVariables.size());
    mLocalVariables[index] = Value::from<T>(value);
  }

  template<CategoryTwoJvmType T>
  void storeValue(types::u2 index, T value)
  {
    assert(index < mLocalVariables.size());
    mLocalVariables[index] = Value::from<T>(value);
    mLocalVariables[index + 1] = Value::from<T>(value);
  }

  void storeGenericValue(types::u2 index, const Value& value)
  {
    assert(index < mLocalVariables.size());
    mLocalVariables[index] = value;
  }

  template<JvmType T>
  T loadValue(types::u2 index)
  {
    return mLocalVariables[index].get<T>();
  }

  // Operand stack
  //==--------------------------------------------------------------------==//
  template<JvmType T>
  void pushOperand(T value)
  {
    if constexpr (StoredAsInt<T>) {
      mOperandStack.push_back(Value::from<int32_t>(static_cast<int32_t>(value)));
    } else {
      mOperandStack.push_back(Value::from<T>(value));
    }
  }

  template<JvmType T>
  T popOperand()
  {
    Value value = popGenericOperand();
    return value.get<T>();
  }

  void pushGenericOperand(Value value)
  {
    Value widenedValue = value.widenToInt();
    mOperandStack.push_back(widenedValue);
  }

  Value popGenericOperand()
  {
    assert(!mOperandStack.empty() && "Cannot pop from an empty operand stack!");
    Value value = mOperandStack.back();
    mOperandStack.pop_back();
    return value;
  }

  Value peek(int32_t entry = 0)
  {
    int32_t idx = static_cast<int32_t>(mOperandStack.size()) - 1 - entry;
    assert(idx >= 0);
    return mOperandStack[idx];
  }

  void replaceStackValue(types::u2 index, Value value)
  {
    mOperandStack.at(index) = value;
  }

  const std::vector<Value>& operandStack() const
  {
    return mOperandStack;
  }

  const std::vector<Value>& locals() const
  {
    return mLocalVariables;
  }

  CallFrame* previous() const
  {
    return mPrevious;
  }

  void clearOperandStack()
  {
    mOperandStack.clear();
  }

  int64_t programCounter() const
  {
    return mProgramCounter;
  }

  void setProgramCounter(int64_t value)
  {
    mProgramCounter = value;
  }

private:
  std::vector<Value> mLocalVariables;
  std::vector<Value> mOperandStack;
  JMethod* mMethod;
  CallFrame* mPrevious;
  int64_t mProgramCounter = 0;
};

} // namespace geevm

#endif // GEEVM_VM_FRAME_H

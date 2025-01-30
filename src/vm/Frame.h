#ifndef GEEVM_VM_FRAME_H
#define GEEVM_VM_FRAME_H

#include "class_file/Opcode.h"
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
      mCode = &mMethod->getCode().bytes();
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

  bool hasNext()
  {
    return mPos < mCode->size();
  }

  int64_t programCounter() const
  {
    return mPos;
  }

  void set(int64_t target)
  {
    // TODO: Check bounds
    mPos = target;
  }

  Opcode next()
  {
    return static_cast<Opcode>((*mCode)[mPos++]);
  }

  types::u1 readU1()
  {
    return (*mCode)[mPos++];
  }

  types::u2 readU2()
  {
    types::u2 value = ((*mCode)[mPos] << 8u) | (*mCode)[mPos + 1];
    mPos += 2;
    return value;
  }

  types::u4 readU4()
  {
    types::u4 value = ((*mCode)[mPos] << 24u) | ((*mCode)[mPos + 1] << 16u) | ((*mCode)[mPos + 2] << 8u) | (*mCode)[mPos + 3];
    mPos += 4;
    return value;
  }

private:
  std::vector<Value> mLocalVariables;
  std::vector<Value> mOperandStack;
  JMethod* mMethod;
  CallFrame* mPrevious;
  int64_t mPos = 0;
  int64_t mOpCodePos = 0;
  const std::vector<types::u1>* mCode;
};

} // namespace geevm

#endif // GEEVM_VM_FRAME_H

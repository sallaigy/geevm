#ifndef GEEVM_VM_FRAME_H
#define GEEVM_VM_FRAME_H

#include "class_file/Opcode.h"
#include "vm/GarbageCollector.h"
#include "vm/Method.h"
#include "vm/Value.h"

#include <cassert>
#include <generator>

namespace geevm
{

class InstanceClass;

class CallFrame
{
public:
  CallFrame(JMethod* method, CallFrame* previous, std::uint64_t* localVariables = nullptr, std::uint64_t* operandStack = nullptr);

  CallFrame(const CallFrame&) = delete;
  CallFrame& operator=(const CallFrame&) = delete;

  ~CallFrame();

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
  void storeValue(size_t index, T value)
  {
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    assert(index < mMethod->getCode().maxLocals());
    mLocalVariables[index] = static_cast<uint64_t>(std::bit_cast<U>(value));
  }

  template<CategoryTwoJvmType T>
  void storeValue(size_t index, T value)
  {
    assert(index < mMethod->getCode().maxLocals());
    assert(index + 1 < mMethod->getCode().maxLocals());
    mLocalVariables[index] = std::bit_cast<uint64_t>(value);
  }

  void storeGenericValue(size_t index, std::uint64_t rawValue)
  {
    assert(index < mMethod->getCode().maxLocals());
    mLocalVariables[index] = rawValue;
  }

  std::pair<uint64_t, bool> loadGenericValue(size_t index)
  {
    assert(index < mMethod->getCode().maxLocals());
    return std::make_pair(mLocalVariables[index], false);
  }

  template<JvmType T>
  T loadValue(size_t index)
  {
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    return std::bit_cast<T>(static_cast<U>(mLocalVariables[index]));
  }

  uint64_t* localVariableAt(size_t index)
  {
    assert(index < mMethod->getCode().maxLocals());
    return &mLocalVariables[index];
  }

  // Operand stack
  //==--------------------------------------------------------------------==//
  template<JvmType T>
  void pushOperand(T value)
  {
    assert(mOperandStackPointer < mMethod->getCode().maxStack());
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    mOperandStack[mOperandStackPointer] = static_cast<uint64_t>(std::bit_cast<U>(value));
    mOperandStackPointer++;

    if constexpr (CategoryTwoJvmType<T>) {
      assert(mOperandStackPointer < mMethod->getCode().maxStack());
      mOperandStackPointer++;
    }
  }

  template<JvmType T>
  T popOperand()
  {
    if constexpr (CategoryTwoJvmType<T>) {
      assert(mOperandStackPointer > 0);
      mOperandStackPointer--;
    }

    assert(mOperandStackPointer > 0);
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    T result = std::bit_cast<T>(static_cast<U>(mOperandStack[mOperandStackPointer - 1]));
    mOperandStackPointer--;

    return result;
  }

  void pushGenericOperand(uint64_t rawValue)
  {
    assert(mOperandStackPointer < mMethod->getCode().maxStack());
    mOperandStack[mOperandStackPointer] = rawValue;
    mOperandStackPointer++;
  }

  Value popGenericOperand()
  {
    assert(mOperandStackPointer > 0 && "Cannot pop from an empty operand stack!");
    Value value{mOperandStack[mOperandStackPointer - 1]};
    mOperandStackPointer--;

    return value;
  }

  uint16_t stackPointer() const
  {
    return mOperandStackPointer;
  }

  void popMultiple(uint16_t count)
  {
    mOperandStackPointer = mOperandStackPointer - count;
  }

  uint64_t* stackElementAt(uint16_t index)
  {
    assert(index < mOperandStackPointer);
    return &mOperandStack[index];
  }

  uint64_t* topOfStack()
  {
    assert(mOperandStackPointer < mMethod->getCode().maxStack());
    return &mOperandStack[mOperandStackPointer];
  }

  void advanceStackPointer(types::u2 count)
  {
    assert(mOperandStackPointer + count <= mMethod->getCode().maxStack());
    mOperandStackPointer += count;
  }

  template<JvmType T>
  T peek(int32_t entry = 0)
  {
    int32_t index = mOperandStackPointer - 1 - entry;
    assert(index >= 0);
    return std::bit_cast<T>(mOperandStack[index]);
  }

  template<JvmType T>
  void replaceStackValue(types::u2 index, T value)
  {
    assert(mOperandStackPointer > index);
    mOperandStack[index] = std::bit_cast<uint64_t>(value);
  }

  /// Prepares another call frame for a function call, passing `numArgs` values from
  /// the operand stack of this frame to the local variable array of the other frame.
  void prepareCall(CallFrame& other, uint16_t numArgs);

  // Iterators
  //==--------------------------------------------------------------------==//

  CallFrame* previous() const
  {
    return mPrevious;
  }

  void clearOperandStack()
  {
    mOperandStackPointer = 0;
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
    return static_cast<Opcode>((mCode)[mPos++]);
  }

  types::u1 readU1()
  {
    return (mCode)[mPos++];
  }

  types::u2 readU2()
  {
    types::u2 value = ((mCode)[mPos] << 8u) | (mCode)[mPos + 1];
    mPos += 2;
    return value;
  }

  types::u4 readU4()
  {
    types::u4 value = ((mCode)[mPos] << 24u) | ((mCode)[mPos + 1] << 16u) | ((mCode)[mPos + 2] << 8u) | (mCode)[mPos + 3];
    mPos += 4;
    return value;
  }

private:
  std::uint64_t* mLocalVariables = nullptr;
  std::uint64_t* mOperandStack = nullptr;
  std::size_t mOperandStackPointer = 0;

  int64_t mPos = 0;
  const types::u1* mCode;
  JMethod* mMethod;
  CallFrame* mPrevious;
};

} // namespace geevm

#endif // GEEVM_VM_FRAME_H

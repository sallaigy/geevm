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
  CallFrame(JMethod* method, CallFrame* previous, std::uint64_t* localVariables = nullptr, bool* localVariableReferences = nullptr,
            std::uint64_t* operandStack = nullptr, bool* operandStackReferences = nullptr);

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
  void storeValue(types::u2 index, T value)
  {
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    assert(index < mMethod->getCode().maxLocals());
    mLocalVariables[index] = static_cast<uint64_t>(std::bit_cast<U>(value));
    if constexpr (std::is_same_v<T, Instance*>) {
      mLocalVariableReferences[index] = true;
    } else {
      mLocalVariableReferences[index] = false;
    }
  }

  template<CategoryTwoJvmType T>
  void storeValue(types::u2 index, T value)
  {
    assert(index < mMethod->getCode().maxLocals());
    assert(index + 1 < mMethod->getCode().maxLocals());
    mLocalVariables[index] = std::bit_cast<uint64_t>(value);
    mLocalVariables[index + 1] = 0;
    mLocalVariableReferences[index] = false;
    mLocalVariableReferences[index + 1] = false;
  }

  void storeGenericValue(types::u2 index, std::uint64_t rawValue, bool isReference)
  {
    assert(index < mMethod->getCode().maxLocals());
    mLocalVariables[index] = rawValue;
    mLocalVariableReferences[index] = isReference;
  }

  std::pair<uint64_t, bool> loadGenericValue(types::u2 index)
  {
    assert(index < mMethod->getCode().maxLocals());
    return std::make_pair(mLocalVariables[index], mLocalVariableReferences[index]);
  }

  template<JvmType T>
  T loadValue(types::u2 index)
  {
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    return std::bit_cast<T>(static_cast<U>(mLocalVariables[index]));
  }

  // Operand stack
  //==--------------------------------------------------------------------==//
  template<JvmType T>
  void pushOperand(T value)
  {
    assert(mOperandStackPointer < mMethod->getCode().maxStack());
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    mOperandStack[mOperandStackPointer] = static_cast<uint64_t>(std::bit_cast<U>(value));
    if constexpr (std::is_same_v<T, Instance*>) {
      mOperandStackReferences[mOperandStackPointer] = true;
    } else {
      mOperandStackReferences[mOperandStackPointer] = false;
    }
    mOperandStackPointer++;

    if constexpr (CategoryTwoJvmType<T>) {
      assert(mOperandStackPointer < mMethod->getCode().maxStack());
      mOperandStack[mOperandStackPointer] = 0;
      mOperandStackReferences[mOperandStackPointer] = false;
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

  void pushGenericOperand(uint64_t rawValue, bool isReference)
  {
    assert(mOperandStackPointer < mMethod->getCode().maxStack());
    mOperandStack[mOperandStackPointer] = rawValue;
    mOperandStackReferences[mOperandStackPointer] = isReference;
    mOperandStackPointer++;
  }

  Value popGenericOperand()
  {
    assert(mOperandStackPointer > 0 && "Cannot pop from an empty operand stack!");
    Value value(mOperandStack[mOperandStackPointer - 1], mOperandStackReferences[mOperandStackPointer - 1]);
    mOperandStackPointer--;

    return value;
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
    if constexpr (std::is_same_v<T, Instance*>) {
      mOperandStackReferences[index] = true;
    } else {
      mOperandStackReferences[index] = false;
    }
  }

  /// Prepares another call frame for a function call, passing `numArgs` values from
  /// the operand stack of this frame to the local variable array of the other frame.
  void prepareCall(CallFrame& other, uint16_t numArgs);

  // Iterators
  //==--------------------------------------------------------------------==//
  std::generator<std::pair<uint16_t, Instance*>> referencesOnStack()
  {
    for (uint16_t i = 0; i < mOperandStackPointer; i++) {
      if (mOperandStackReferences[i]) {
        co_yield std::make_pair(i, std::bit_cast<Instance*>(mOperandStack[i]));
      }
    }
  }

  std::generator<std::pair<uint16_t, Instance*>> referencesInLocals()
  {
    for (uint16_t i = 0; i < mMethod->getCode().maxLocals(); i++) {
      if (mLocalVariableReferences[i]) {
        co_yield std::make_pair(i, std::bit_cast<Instance*>(mLocalVariables[i]));
      }
    }
  }

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
  bool* mLocalVariableReferences = nullptr;
  std::uint64_t* mOperandStack = nullptr;
  bool* mOperandStackReferences = nullptr;
  std::uint16_t mOperandStackPointer = 0;

  int64_t mPos = 0;
  const types::u1* mCode;
  JMethod* mMethod;
  CallFrame* mPrevious;
};

} // namespace geevm

#endif // GEEVM_VM_FRAME_H

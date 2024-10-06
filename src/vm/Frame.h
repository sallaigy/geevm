#ifndef GEEVM_VM_FRAME_H
#define GEEVM_VM_FRAME_H

#include "common/JvmTypes.h"
#include "vm/Instance.h"
#include "vm/Method.h"

#include <cassert>

namespace geevm
{

class JClass;

class Value
{
public:
  enum class Kind
  {
    Byte,
    Short,
    Int,
    Long,
    Char,
    Float,
    Double,
    ReturnAddress,
    Reference
  };

private:
  using Storage = struct
  {
    Kind kind;
    union
    {
      std::int8_t mByte;
      std::int16_t mShort;
      std::int32_t mInt;
      std::int64_t mLong;
      char16_t mChar;
      float mFloat;
      double mDouble;
      std::uint32_t mReturnAddress;
      Instance* mReference;
    } data;
  };

private:
  explicit Value(Storage storage)
    : mStorage(storage)
  {
  }

public:
  Value(const Value& other) = default;

#define GEEVM_VM_VALUE_CONSTRUCTOR(TYPE)                   \
  static Value TYPE(decltype(Storage::data.m##TYPE) value) \
  {                                                        \
    Storage storage;                                       \
    storage.kind = Kind::TYPE;                             \
    storage.data.m##TYPE = value;                          \
    return Value(storage);                                 \
  }

  GEEVM_VM_VALUE_CONSTRUCTOR(Byte)
  GEEVM_VM_VALUE_CONSTRUCTOR(Short)
  GEEVM_VM_VALUE_CONSTRUCTOR(Int)
  GEEVM_VM_VALUE_CONSTRUCTOR(Long)
  GEEVM_VM_VALUE_CONSTRUCTOR(Char)
  GEEVM_VM_VALUE_CONSTRUCTOR(Float)
  GEEVM_VM_VALUE_CONSTRUCTOR(Double)
  GEEVM_VM_VALUE_CONSTRUCTOR(ReturnAddress)
  GEEVM_VM_VALUE_CONSTRUCTOR(Reference)

  Kind kind() const
  {
    return mStorage.kind;
  }

  std::int32_t asInt() const
  {
    assert(mStorage.kind == Kind::Int);
    return mStorage.data.mInt;
  }

  std::int64_t asLong() const
  {
    assert(mStorage.kind == Kind::Long);
    return mStorage.data.mLong;
  }

  Instance* asReference() const
  {
    assert(mStorage.kind == Kind::Reference);
    return mStorage.data.mReference;
  }

private:
  Storage mStorage;
};

class CallFrame
{
public:
  explicit CallFrame(JClass* klass, JMethod* method)
    : mClass(klass), mMethod(method), mLocalVariables(method->getCode().maxLocals(), Value::Int(0))
  {
  }

  JClass* currentClass()
  {
    return mClass;
  }

  // Local variable manipulation
  //==--------------------------------------------------------------------==//
  void storeValue(types::u2 index, Value value)
  {
    assert(index < mLocalVariables.size());
    mLocalVariables[index] = value;
  }

  void storeLongValue(types::u2 index, Value value)
  {
    assert(value.kind() == Value::Kind::Long || value.kind() == Value::Kind::Double);
    mLocalVariables[index] = value;
    mLocalVariables[index + 1] = value;
  }

  Value loadValue(types::u2 index)
  {
    return mLocalVariables[index];
  }

  // Operand stack
  //==--------------------------------------------------------------------==//
  void pushOperand(Value value)
  {
    mOperandStack.push_back(value);
  }

  Value popOperand()
  {
    Value value = mOperandStack.back();
    mOperandStack.pop_back();
    return value;
  }

  Value popInt()
  {
    return popOperand();
  }

  Value popLong()
  {
    Value value = popOperand();
    assert(value.kind() == Value::Kind::Long);
    return value;
  }

  Value peek(int entry = 0)
  {
    int idx = mOperandStack.size() - 1 - entry;
    assert(idx >= 0);
    return mOperandStack[idx];
  }

private:
  std::vector<Value> mLocalVariables;
  std::vector<Value> mOperandStack;
  JClass* mClass;
  JMethod* mMethod;
};

} // namespace geevm

#endif // GEEVM_VM_FRAME_H

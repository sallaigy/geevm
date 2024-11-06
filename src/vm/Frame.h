#ifndef GEEVM_VM_FRAME_H
#define GEEVM_VM_FRAME_H

#include "common/JvmTypes.h"
#include "vm/Method.h"

#include <cassert>

namespace geevm
{
class InstanceClass;
}
namespace geevm
{

class JClass;
class Instance;

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

  char16_t asChar() const
  {
    assert(mStorage.kind == Kind::Char);
    return mStorage.data.mChar;
  }

  float asFloat() const
  {
    assert(mStorage.kind == Kind::Float);
    return mStorage.data.mFloat;
  }

  double asDouble() const
  {
    assert(mStorage.kind == Kind::Double);
    return mStorage.data.mDouble;
  }

  Instance* asReference() const
  {
    assert(mStorage.kind == Kind::Reference);
    return mStorage.data.mReference;
  }

  bool isCategoryTwo() const
  {
    return mStorage.kind == Kind::Double || mStorage.kind == Kind::Long;
  }

  static Value defaultValue(const FieldType& fieldType)
  {
    if (auto primitiveType = fieldType.asPrimitive(); primitiveType) {
      switch (*primitiveType) {
        case PrimitiveType::Byte: return Value::Byte(0);
        case PrimitiveType::Char: return Value::Char(0);
        case PrimitiveType::Double: return Value::Double(0);
        case PrimitiveType::Float: return Value::Float(0);
        case PrimitiveType::Int: return Value::Int(0);
        case PrimitiveType::Long: return Value::Long(0);
        case PrimitiveType::Short: return Value::Short(0);
        case PrimitiveType::Boolean: return Value::Int(0);
      }
    } else if (fieldType.asObjectName().has_value()) {
      return Value::Reference(nullptr);
    }

    assert(false && "Unknown field type!");
  }

private:
  Storage mStorage;
};

class CallFrame
{
public:
  explicit CallFrame(JMethod* method, CallFrame* previous)
    : mMethod(method), mLocalVariables(method->getCode().maxLocals(), Value::Int(0)), mPrevious(previous)
  {
  }

  InstanceClass* currentClass()
  {
    return mMethod->getClass();
  }

  JMethod* currentMethod()
  {
    return mMethod;
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
    assert(!mOperandStack.empty() && "Cannot pop from an empty operand stack!");
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

private:
  std::vector<Value> mLocalVariables;
  std::vector<Value> mOperandStack;
  JMethod* mMethod;
  CallFrame* mPrevious;
};

} // namespace geevm

#endif // GEEVM_VM_FRAME_H

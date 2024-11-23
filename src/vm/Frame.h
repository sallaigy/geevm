#ifndef GEEVM_VM_FRAME_H
#define GEEVM_VM_FRAME_H

#include "common/JvmTypes.h"
#include "common/TypeTraits.h"
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

template<class T>
concept JvmType = is_one_of<T, std::int8_t, std::int16_t, std::int32_t, std::int64_t, char16_t, float, double, std::uint32_t, Instance*>();

template<class T>
concept CategoryTwoJvmType = is_one_of<T, std::int64_t, double>();

template<class T>
concept CategoryOneJvmType = JvmType<T> && !CategoryTwoJvmType<T>;

/// JVM types that are sign-extended to int before being pushed to the operand stack.
template<class T>
concept StoredAsInt = JvmType<T> && is_one_of<T, std::int8_t, std::int16_t, char16_t>();

class Value
{
public:
private:
  using Storage = std::variant<std::int8_t, std::int16_t, std::int32_t, std::int64_t, char16_t, float, double, Instance*>;

private:
  template<JvmType T>
  explicit Value(T value)
    : mStorage(value)
  {
  }

public:
  template<JvmType T>
  static Value from(T value)
  {
    return Value(value);
  }

  template<JvmType T>
  T get() const
  {
    return std::get<T>(mStorage);
  }

public:
  Value(const Value& other) = default;
  Value& operator=(const Value& other) = default;

  bool isCategoryTwo() const
  {
    return std::holds_alternative<int64_t>(mStorage) || std::holds_alternative<double>(mStorage);
  }

  static Value defaultValue(const FieldType& fieldType)
  {
    if (auto primitiveType = fieldType.asPrimitive(); primitiveType) {
      switch (*primitiveType) {
        case PrimitiveType::Byte: return Value(static_cast<std::int8_t>(0));
        case PrimitiveType::Char: return Value(static_cast<char16_t>(0));
        case PrimitiveType::Double: return Value(static_cast<double>(0));
        case PrimitiveType::Float: return Value(static_cast<float>(0));
        case PrimitiveType::Int: return Value(static_cast<std::int32_t>(0));
        case PrimitiveType::Long: return Value(static_cast<std::int64_t>(0));
        case PrimitiveType::Short: return Value(static_cast<std::int16_t>(0));
        case PrimitiveType::Boolean: return Value(static_cast<std::int32_t>(0));
      }
    } else if (fieldType.asObjectName().has_value()) {
      return Value(static_cast<Instance*>(nullptr));
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
    : mMethod(method), mLocalVariables(method->getCode().maxLocals(), Value::from<int32_t>(0)), mPrevious(previous)
  {
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
    mOperandStack.push_back(Value::from<T>(value));
  }

  template<JvmType T>
  T popOperand()
  {
    Value value = popGenericOperand();
    return value.get<T>();
  }

  void pushGenericOperand(Value value)
  {
    mOperandStack.push_back(value);
  }

  Value popGenericOperand()
  {
    assert(!mOperandStack.empty() && "Cannot pop from an empty operand stack!");
    Value value = mOperandStack.back();
    mOperandStack.pop_back();
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

  // Exceptions
  //==--------------------------------------------------------------------==//

private:
  std::vector<Value> mLocalVariables;
  std::vector<Value> mOperandStack;
  JMethod* mMethod;
  CallFrame* mPrevious;
};

} // namespace geevm

#endif // GEEVM_VM_FRAME_H

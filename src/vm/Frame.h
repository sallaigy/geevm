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

class Value
{
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

  bool operator==(const Value&) const = default;

  Value widenToInt()
  {
    return std::visit(
        [this](auto&& arg) -> Value {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (StoredAsInt<T>) {
            return Value::from(static_cast<int32_t>(arg));
          } else {
            return *this;
          }
        },
        mStorage);
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
    return fieldType.map(
        []<PrimitiveType Type>() {
          return Value::from<typename PrimitiveTypeTraits<Type>::Representation>(0);
        },
        [](types::JStringRef name) {
          return Value::from<Instance*>(nullptr);
        },
        [](const ArrayType& array) {
          return Value::from<Instance*>(nullptr);
        });
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

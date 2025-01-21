#ifndef GEEVM_VM_VALUE_H
#define GEEVM_VM_VALUE_H

#include "class_file/Descriptor.h"
#include "common/JvmTypes.h"

#include <variant>

namespace geevm
{

class Value
{
  using Storage = std::variant<std::int8_t, std::int16_t, std::int32_t, std::int64_t, char16_t, float, double, Instance*>;

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
    if constexpr (StoredAsInt<T>) {
      if (std::holds_alternative<int32_t>(mStorage)) {
        return static_cast<T>(std::get<int32_t>(mStorage));
      }
    }
    return std::get<T>(mStorage);
  }

  bool operator==(const Value&) const = default;

  Value widenToInt()
  {
    return std::visit([this](auto&& arg) -> Value {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (StoredAsInt<T>) {
        return Value::from(static_cast<int32_t>(arg));
      } else {
        return *this;
      }
    }, mStorage);
  }

public:
  Value(const Value& other) = default;
  Value& operator=(const Value& other) = default;

  bool isCategoryTwo() const
  {
    return std::holds_alternative<int64_t>(mStorage) || std::holds_alternative<double>(mStorage);
  }

  template<JvmType T>
  std::optional<T> tryGet() const
  {
    if (std::holds_alternative<T>(mStorage)) {
      return std::get<T>(mStorage);
    }

    return std::nullopt;
  }

  static Value defaultValue(const FieldType& fieldType)
  {
    return fieldType.map([]<PrimitiveType Type>() {
      return Value::from<typename PrimitiveTypeTraits<Type>::Representation>(0);
    }, [](types::JStringRef name) {
      return Value::from<Instance*>(nullptr);
    }, [](const ArrayType& array) {
      return Value::from<Instance*>(nullptr);
    });
  }

private:
  Storage mStorage;
};

} // namespace geevm

#endif // GEEVM_VM_VALUE_H

#ifndef GEEVM_VM_VALUE_H
#define GEEVM_VM_VALUE_H

#include "class_file/Descriptor.h"
#include "common/JvmTypes.h"

namespace geevm
{

// Helper template
template<std::size_t N>
struct unsigned_type_of_length
{
};

template<>
struct unsigned_type_of_length<8>
{
  using type = std::uint8_t;
};

template<>
struct unsigned_type_of_length<16>
{
  using type = std::uint16_t;
};

template<>
struct unsigned_type_of_length<32>
{
  using type = std::uint32_t;
};

template<>
struct unsigned_type_of_length<64>
{
  using type = std::uint64_t;
};

class Value
{
  struct Storage
  {
    std::uint64_t value;
    bool isReference;
  };

public:
  Value(std::uint64_t value, bool isReference)
    : mStorage(value, isReference)
  {
  }

  Value(const Value& other) = default;
  Value& operator=(const Value& other) = default;

  template<JvmType T>
  static Value from(T value)
  {
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    U valueAsU = std::bit_cast<U>(value);

    if constexpr (std::is_same_v<T, Instance*>) {
      return Value(static_cast<uint64_t>(valueAsU), true);
    } else {
      return Value(static_cast<uint64_t>(valueAsU), false);
    }
  }

  template<JvmType T>
  static Value fromRaw(uint64_t rawValue)
  {
    if constexpr (std::is_same_v<T, Instance*>) {
      return Value::from<Instance*>(std::bit_cast<Instance*>(rawValue));
    } else {
      using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
      return Value::from<T>(std::bit_cast<T>(static_cast<U>(rawValue)));
    }
  }

  template<JvmType T>
  T get() const
  {
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;

    return std::bit_cast<T>(static_cast<U>(mStorage.value));
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

  std::pair<std::uint64_t, bool> toRaw() const
  {
    return std::make_pair(mStorage.value, mStorage.isReference);
  }

private:
  Storage mStorage;
};

} // namespace geevm

#endif // GEEVM_VM_VALUE_H

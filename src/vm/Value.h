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
public:
  explicit Value(std::uint64_t value)
    : mStorage(value)
  {
  }

  Value(const Value& other) = default;
  Value& operator=(const Value& other) = default;

  template<JvmType T>
  static Value from(T value)
  {
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    U valueAsU = std::bit_cast<U>(value);

    return Value(static_cast<uint64_t>(valueAsU));
  }

  template<JvmType T>
  T get() const
  {
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    return std::bit_cast<T>(static_cast<U>(mStorage));
  }

  static Value defaultValue(const FieldType& fieldType)
  {
    return Value(0);
  }

  std::uint64_t toRaw() const
  {
    return mStorage;
  }

private:
  std::uint64_t mStorage;
};

} // namespace geevm

#endif // GEEVM_VM_VALUE_H

#ifndef GEEVM_CLASS_FILE_DESCRIPTOR_H
#define GEEVM_CLASS_FILE_DESCRIPTOR_H

#include "common/JvmTypes.h"

#include <common/Debug.h>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace geevm
{

class ArrayType;

class FieldType
{
  using StorageTy = std::variant<PrimitiveType, types::JString>;

public:
  explicit FieldType(StorageTy storage, int arrayDimensions = 0)
    : mVariant(std::move(storage)), mDimensions(arrayDimensions)
  {
  }

  static std::optional<FieldType> parse(types::JStringRef input);

  bool operator==(const FieldType&) const = default;

  std::optional<PrimitiveType> asPrimitive() const
  {
    if (std::holds_alternative<PrimitiveType>(mVariant) && mDimensions == 0) {
      return std::get<PrimitiveType>(mVariant);
    }

    return std::nullopt;
  }

  std::optional<types::JString> asReference() const
  {
    if (std::holds_alternative<types::JString>(mVariant) && mDimensions == 0) {
      return std::get<types::JString>(mVariant);
    }

    return std::nullopt;
  }

  std::optional<ArrayType> asArrayType() const;

  bool isReferenceOrArray() const
  {
    return std::holds_alternative<types::JString>(mVariant) || mDimensions != 0;
  }

  bool isCategoryTwo() const;

  std::size_t sizeOf() const;

  /// Map the contents of this descriptor using one of the provided mapper functions.
  /// \param primitiveMapper a template function with a PrimitiveType template parameter, mapping primitive values.
  /// \param classMapper a function that maps a class name.
  /// \param arrayMapper a function that maps an ArrayType.
  template<class PrimitiveMapFunc, class ClassMapFunc, class ArrayMapFunc>
  decltype(auto) map(const PrimitiveMapFunc& primitiveMapper, const ClassMapFunc& classMapper, const ArrayMapFunc& arrayMapper) const;

  types::JString toJavaString() const;

protected:
  StorageTy mVariant;
  int mDimensions;
};

class ArrayType : public FieldType
{
public:
  explicit ArrayType(FieldType elementType)
    : FieldType(std::move(elementType))
  {
  }

  FieldType getElementType() const
  {
    return FieldType(mVariant, mDimensions - 1);
  }

  int getDimensions() const
  {
    return mDimensions;
  }

  types::JString className() const;
};

class ReturnType
{
  explicit ReturnType()
    : mIsVoid(true), mType(PrimitiveType::Boolean)
  {
  }

public:
  static ReturnType VoidType;

  explicit ReturnType(FieldType type)
    : mType(type)
  {
  }

  bool operator==(const ReturnType&) const = default;

  bool isVoid() const
  {
    return mIsVoid;
  }

  FieldType getType() const
  {
    return mType;
  }

  types::JString toJavaString() const;

private:
  bool mIsVoid = false;
  FieldType mType;
};

class MethodDescriptor
{
public:
  MethodDescriptor(ReturnType returnType, std::vector<FieldType> parameters)
    : mReturnType(returnType), mParameterTypes(std::move(parameters))
  {
  }

  static std::optional<MethodDescriptor> parse(types::JStringRef input);

  bool operator==(const MethodDescriptor&) const = default;

  const std::vector<FieldType>& parameters() const
  {
    return mParameterTypes;
  }

  const ReturnType& returnType() const
  {
    return mReturnType;
  }

  types::JString formatAsJavaSignature(const types::JString& name) const;

  size_t numParameterSlots() const;

private:
  ReturnType mReturnType;
  std::vector<FieldType> mParameterTypes;
};

template<class PrimitiveMapFunc, class ClassMapFunc, class ArrayMapFunc>
decltype(auto) FieldType::map(const PrimitiveMapFunc& primitiveMapper, const ClassMapFunc& classMapper, const ArrayMapFunc& arrayMapper) const
{
  if (auto primitive = this->asPrimitive(); primitive) {
    switch (*primitive) {
      case PrimitiveType::Byte: return primitiveMapper.template operator()<PrimitiveType::Byte>();
      case PrimitiveType::Char: return primitiveMapper.template operator()<PrimitiveType::Char>();
      case PrimitiveType::Double: return primitiveMapper.template operator()<PrimitiveType::Double>();
      case PrimitiveType::Float: return primitiveMapper.template operator()<PrimitiveType::Float>();
      case PrimitiveType::Int: return primitiveMapper.template operator()<PrimitiveType::Int>();
      case PrimitiveType::Long: return primitiveMapper.template operator()<PrimitiveType::Long>();
      case PrimitiveType::Short: return primitiveMapper.template operator()<PrimitiveType::Short>();
      case PrimitiveType::Boolean: return primitiveMapper.template operator()<PrimitiveType::Boolean>();
    }
    GEEVM_UNREACHBLE("Unknown primitive type");
  }

  if (auto array = this->asArrayType(); array) {
    return arrayMapper(*array);
  }

  if (auto reference = this->asReference(); reference) {
    return classMapper(*reference);
  }

  GEEVM_UNREACHBLE("Unknown descriptor type");
}

} // namespace geevm

#endif

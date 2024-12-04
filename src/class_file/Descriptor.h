#ifndef GEEVM_CLASS_FILE_DESCRIPTOR_H
#define GEEVM_CLASS_FILE_DESCRIPTOR_H

#include "common/JvmTypes.h"

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace geevm
{

enum class PrimitiveType
{
  Byte,
  Char,
  Double,
  Float,
  Int,
  Long,
  Short,
  Boolean,
};

class FieldType
{
public:
  explicit FieldType(PrimitiveType primitiveType, int arrayDimensions = 0)
    : mVariant(primitiveType), mDimensions(arrayDimensions)
  {
  }

  explicit FieldType(types::JString className, int arrayDimensions = 0)
    : mVariant(className), mDimensions(arrayDimensions)
  {
  }

  static std::optional<FieldType> parse(types::JStringRef input);

  bool operator==(const FieldType&) const = default;

  std::optional<PrimitiveType> asPrimitive() const
  {
    if (std::holds_alternative<PrimitiveType>(mVariant)) {
      return std::get<PrimitiveType>(mVariant);
    }

    return std::nullopt;
  }

  std::optional<types::JString> asObjectName() const
  {
    if (std::holds_alternative<types::JString>(mVariant)) {
      return std::get<types::JString>(mVariant);
    }
    return std::nullopt;
  }

  std::size_t sizeOf() const;

  template<class R>
  R map(std::function<R(const PrimitiveType&)> primitiveMapper, std::function<R(const types::JString&)> classNameMapper) const
  {
    return std::visit(
        [&](auto&& arg) {
          if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, types::JString>) {
            return classNameMapper(arg);
          } else {
            return primitiveMapper(arg);
          }
        },
        mVariant);
  }

  int dimensions() const
  {
    return mDimensions;
  }

  types::JString toJavaString() const;

private:
  std::variant<PrimitiveType, types::JString> mVariant;
  int mDimensions;
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

private:
  ReturnType mReturnType;
  std::vector<FieldType> mParameterTypes;
};

} // namespace geevm

#endif

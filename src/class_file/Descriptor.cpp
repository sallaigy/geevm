#include "class_file/Descriptor.h"

#include <algorithm>
#include <sstream>

using namespace geevm;

ReturnType ReturnType::VoidType{};

static std::optional<std::pair<int, FieldType>> parseField(types::JStringRef input)
{
  // Syntax:
  //  FieldType -> BaseType | ObjectType | ArrayType
  //  BaseType -> 'B' | 'C' | ...
  //  ArrayType -> '[' FieldType
  int pos = 0;
  int arrayDimensions = 0;

  while (pos < input.size() && input[pos] == u'[') {
    arrayDimensions += 1;
    pos += 1;
  }

  if (pos >= input.size()) {
    return std::nullopt;
  }

  if (char16_t c = input[pos]; c == u'L') {
    // Find the position of the ';'
    auto endPos = input.find(u';', pos);
    if (endPos == types::JStringRef::npos) {
      return std::nullopt;
    }

    auto name = input.substr(pos + 1, endPos - pos - 1);
    FieldType type(types::JString{name}, arrayDimensions);

    return std::make_optional<>(std::make_pair<>(endPos + 1, type));
  } else {
    PrimitiveType primitiveType;

    switch (c) {
      case 'B': primitiveType = PrimitiveType::Byte; break;
      case 'C': primitiveType = PrimitiveType::Char; break;
      case 'D': primitiveType = PrimitiveType::Double; break;
      case 'F': primitiveType = PrimitiveType::Float; break;
      case 'I': primitiveType = PrimitiveType::Int; break;
      case 'J': primitiveType = PrimitiveType::Long; break;
      case 'S': primitiveType = PrimitiveType::Short; break;
      case 'Z': primitiveType = PrimitiveType::Boolean; break;
      default:
        // Cannot input the type - character is not a valid primitive type.
        return std::nullopt;
    }

    return std::make_optional(std::make_pair(pos + 1, FieldType{primitiveType, arrayDimensions}));
  }
}

std::optional<FieldType> FieldType::parse(types::JStringRef input)
{
  auto result = parseField(input);
  if (result && result->first == input.length()) {
    return std::make_optional<>(result->second);
  }

  return std::nullopt;
}

std::optional<MethodDescriptor> MethodDescriptor::parse(types::JStringRef input)
{
  // Method -> '(' Parameter* ')' Return
  // Parameter -> FieldType
  // Return -> FieldType | 'V'

  int pos = 0;
  bool parametersFinished = false;
  if (input.empty() || input[pos] != u'(') {
    return std::nullopt;
  }

  pos += 1;
  std::vector<FieldType> parameters;
  while (pos < input.size()) {
    auto c = input[pos];
    if (c == u')') {
      pos += 1;
      parametersFinished = true;
      break;
    }

    // Read a field
    auto fieldData = parseField(input.substr(pos));
    if (!fieldData) {
      return std::nullopt;
    }

    parameters.push_back(fieldData->second);
    pos += fieldData->first;
  }

  if (!parametersFinished || pos >= input.size()) {
    return std::nullopt;
  }

  // Parse the return type
  if (input[pos] == u'V') {
    return std::make_optional<>(MethodDescriptor{ReturnType::VoidType, parameters});
  }

  auto returnTy = parseField(input.substr(pos));
  if (returnTy) {
    return std::make_optional(MethodDescriptor{ReturnType{returnTy->second}, parameters});
  }

  return std::nullopt;
}

types::JString FieldType::toJavaString() const
{
  types::JString str;

  return map([]<PrimitiveType Type>() {
    return types::JString{PrimitiveTypeTraits<Type>::Name};
  }, [](const types::JString& className) {
    auto buff = types::JString{className};
    std::ranges::replace(buff, u'/', u'.');
    return buff;
  }, [](const ArrayType& array) {
    auto elementString = array.getElementType().toJavaString();
    for (size_t i = 0; i < array.getDimensions(); i++) {
      elementString += u"[]";
    }
    return elementString;
  });
}

std::optional<ArrayType> FieldType::asArrayType() const
{
  if (mDimensions != 0) {
    return ArrayType(*this);
  }
  return std::nullopt;
}

types::JString ReturnType::toJavaString() const
{
  if (this->isVoid()) {
    return u"void";
  }

  return this->getType().toJavaString();
}

types::JString MethodDescriptor::formatAsJavaSignature(const types::JString& name) const
{
  types::JString str;
  str += this->returnType().toJavaString();
  str += u" " + name;

  str += u"(";
  for (size_t i = 0; i < this->parameters().size(); i++) {
    str += this->parameters()[i].toJavaString();
    if (i != this->parameters().size() - 1) {
      str += u", ";
    }
  }

  str += u")";
  return str;
}

std::size_t FieldType::sizeOf() const
{
  return this->map([]<PrimitiveType Type>() {
    return sizeof(typename PrimitiveTypeTraits<Type>::Representation);
  }, [](types::JStringRef _) {
    return sizeof(void*);
  }, [](const ArrayType& _) {
    return sizeof(void*);
  });
}

types::JString ArrayType::className() const
{
  types::JString buff = u"[";

  auto elementTy = getElementType();
  buff += elementTy.map([]<PrimitiveType Type>() -> types::JString {
    return types::JString{PrimitiveTypeTraits<Type>::Descriptor};
  }, [](types::JStringRef className) -> types::JString {
    return u"L" + types::JString{className} + u";";
  }, [](const ArrayType& array) -> types::JString {
    return array.className();
  });

  return buff;
}

bool FieldType::isCategoryTwo() const
{
  return this->map([]<PrimitiveType Type>() {
    using T = typename PrimitiveTypeTraits<Type>::Representation;
    if constexpr (CategoryTwoJvmType<T>) {
      return true;
    } else {
      return false;
    }
  }, [](types::JStringRef) {
    return false;
  }, [](const ArrayType&) {
    return false;
  });
}

size_t MethodDescriptor::numParameterSlots() const
{
  size_t count = 0;
  for (const auto& param : this->parameters()) {
    count++;
    if (param.isCategoryTwo()) {
      count++;
    }
  }

  return count;
}

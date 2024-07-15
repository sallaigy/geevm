#include "class_file/Descriptor.h"

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

  if (!parametersFinished) {
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

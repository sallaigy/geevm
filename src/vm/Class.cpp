#include "vm/Class.h"

#include "Vm.h"

#include <utility>

using namespace geevm;

JClass::JClass(std::unique_ptr<ClassFile> classFile)
  : mClassFile(std::move(classFile))
{
}

void JClass::prepare()
{
  if (mIsPrepared) {
    return;
  }

  // Initialize the static fields map
  for (const FieldInfo& field : mClassFile->fields()) {
    if (hasAccessFlag(field.accessFlags(), FieldAccessFlags::ACC_STATIC)) {
      auto fieldName = mClassFile->constantPool().getString(field.nameIndex());
      auto initialValue =
          FieldType::parse(mClassFile->constantPool().getString(field.descriptorIndex())).and_then([this](const FieldType& fieldType) -> std::optional<Value> {
            if (auto primitiveType = fieldType.asPrimitive(); primitiveType.has_value()) {
              switch (*primitiveType) {
                case PrimitiveType::Int: return Value::Int(0);
                case PrimitiveType::Char: return Value::Char(0);
                case PrimitiveType::Byte: return Value::Byte(0);
                case PrimitiveType::Short: return Value::Short(0);
                case PrimitiveType::Boolean: return Value::Int(0);
                case PrimitiveType::Float: return Value::Float(0);
                case PrimitiveType::Long: return Value::Long(0);
                case PrimitiveType::Double: return Value::Double(0);
              }
            }

            return std::nullopt;
          });

      if (initialValue.has_value()) {
        mStaticFields.try_emplace(fieldName, *initialValue);
      } else {
        // Should always be present
        std::unreachable();
      }
    }
  }

  for (const MethodInfo& method : mClassFile->methods()) {
    types::JStringRef name = mClassFile->constantPool().getString(method.nameIndex());
    types::JStringRef descriptor = mClassFile->constantPool().getString(method.descriptorIndex());

    auto parsedDescriptor = MethodDescriptor::parse(descriptor);
    // TODO: Verification error
    assert(parsedDescriptor.has_value() && "Cannot parse descriptor");

    mMethods.try_emplace(NameAndDescriptor{name, descriptor}, std::make_unique<JMethod>(method, *parsedDescriptor));
  }

  mIsPrepared = true;
}

void JClass::initialize(Vm& vm)
{
  if (mIsInitialized || mIsUnderInitialization) {
    return;
  }
  mIsUnderInitialization = true;

  // TODO: Initialize superclass, superinterfaces

  for (const FieldInfo& field : mClassFile->fields()) {
    if (hasAccessFlag(field.accessFlags(), FieldAccessFlags::ACC_STATIC) && hasAccessFlag(field.accessFlags(), FieldAccessFlags::ACC_FINAL)) {
      auto fieldName = mClassFile->constantPool().getString(field.nameIndex());
      mStaticFields.emplace(fieldName, getInitialFieldValue(field));
    }
  }

  auto clsInit = getMethod(u"<clinit>", u"()V");
  if (clsInit != nullptr) {
    vm.execute(this, clsInit);
  }

  mIsInitialized = true;
  mIsUnderInitialization = false;
}

Value JClass::getInitialFieldValue(const FieldInfo& field)
{
  auto descriptor = FieldType::parse(mClassFile->constantPool().getString(field.descriptorIndex()));
  assert(descriptor.has_value() && "The field descriptor should be valid!");

  std::optional<Value> value = field.constantValue().and_then([this, &descriptor](types::u2 cvIndex) -> std::optional<Value> {
    auto& entry = mClassFile->constantPool().getEntry(cvIndex);
    if (auto primitiveType = descriptor->asPrimitive(); primitiveType.has_value()) {
      switch (*primitiveType) {
        case PrimitiveType::Int: return Value::Int(entry.data.singleInteger);
        case PrimitiveType::Char: return Value::Char(static_cast<char16_t>(entry.data.singleInteger));
        case PrimitiveType::Byte: return Value::Byte(static_cast<int8_t>(entry.data.singleInteger));
        case PrimitiveType::Short: return Value::Short(static_cast<int16_t>(entry.data.singleInteger));
        case PrimitiveType::Boolean: return Value::Int(entry.data.singleInteger);
        case PrimitiveType::Float: return Value::Float(entry.data.singleFloat);
        case PrimitiveType::Long: return Value::Long(entry.data.doubleInteger);
        case PrimitiveType::Double: return Value::Double(entry.data.doubleFloat);
      }
      // No other primitive types are possible
      std::unreachable();
    }

    if (auto objectName = descriptor->asObjectName(); objectName && *objectName == u"Ljava/lang/String;") {
      // TODO: Handle String types
    }

    return std::nullopt;
  });

  if (value) {
    return *value;
  }

  assert("There should a ConstantValue attribute for a static field!");
  std::unreachable();
}

JMethod* JClass::getMethod(const types::JString& name, const types::JString& descriptor)
{
  NameAndDescriptor pair{name, descriptor};
  if (auto it = mMethods.find(pair); it != mMethods.end()) {
    return it->second.get();
  }

  // TODO: Return JvmExpected?
  return nullptr;
}

MethodRef JClass::getMethodRef(types::u2 index)
{
  auto& entry = mClassFile->constantPool().getEntry(index);
  assert(entry.tag == ConstantPool::Tag::CONSTANT_Methodref && "Can only fetch a method ref from a method ref entry!");

  types::JString className{mClassFile->constantPool().getClassName(entry.data.classAndNameRef.classIndex)};
  auto [methodName, descriptor] = mClassFile->constantPool().getNameAndType(entry.data.classAndNameRef.nameAndTypeIndex);

  return MethodRef{className, types::JString{methodName}, types::JString{descriptor}};
}

const FieldRef& JClass::getFieldRef(types::u2 index)
{
  if (auto it = mFieldRefCache.find(index); it != mFieldRefCache.end()) {
    return it->second;
  }

  auto& entry = mClassFile->constantPool().getEntry(index);
  assert(entry.tag == ConstantPool::Tag::CONSTANT_Fieldref && "Can only fetch a method ref from a method ref entry!");

  types::JString className{mClassFile->constantPool().getClassName(entry.data.classAndNameRef.classIndex)};
  auto [fieldName, descriptor] = mClassFile->constantPool().getNameAndType(entry.data.classAndNameRef.nameAndTypeIndex);

  auto result = mFieldRefCache.emplace(index, FieldRef{className, types::JString{fieldName}, types::JString{descriptor}});

  return result.first->second;
}

std::optional<types::JStringRef> JClass::superClass() const
{
  return this->constantPool().getOptionalClassName(mClassFile->superClass());
}

std::vector<types::JStringRef> JClass::interfaces() const
{
  std::vector<types::JStringRef> result;
  for (types::u2 interfaceIndex : mClassFile->interfaces()) {
    result.push_back(this->constantPool().getClassName(interfaceIndex));
  }
  return result;
}

Value JClass::getStaticField(types::JStringRef name)
{
  return mStaticFields.at(name);
}

void JClass::storeStaticField(types::JStringRef name, Value value)
{
  mStaticFields.at(name) = value;
}

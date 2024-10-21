#include "vm/Class.h"

#include "Vm.h"

#include <utility>

using namespace geevm;

JClass::JClass(std::unique_ptr<ClassFile> classFile)
  : mClassFile(std::move(classFile))
{
  if (mClassFile != nullptr) {
    mClassName = constantPool().getClassName(mClassFile->thisClass());
  }
}

void JClass::prepare(BootstrapClassLoader& classLoader)
{
  if (mIsPrepared) {
    return;
  }

  if (auto superClass = this->superClass(); superClass) {
    // TODO: Check errors
    auto loaded = classLoader.loadClass(types::JString{*superClass});
    assert(loaded);

    mSuperClass = *loaded;
  }

  for (types::JStringRef interfaceName : this->interfaces()) {
    // TODO: Check errors
    mSuperInterfaces.push_back(*classLoader.loadClass(types::JString{interfaceName}));
  }

  if (mSuperClass != nullptr) {
    for (auto& [name, value] : mSuperClass->fields()) {
      mFields.try_emplace(name, std::make_unique<JField>(value->fieldInfo(), value->fieldType()));
    }
  }

  for (const FieldInfo& field : mClassFile->fields()) {
    auto fieldName = mClassFile->constantPool().getString(field.nameIndex());
    auto descriptor = mClassFile->constantPool().getString(field.descriptorIndex());
    auto fieldType = FieldType::parse(descriptor);
    assert(fieldType.has_value());

    if (hasAccessFlag(field.accessFlags(), FieldAccessFlags::ACC_STATIC)) {
      Value initialValue = Value::defaultValue(*fieldType);
      mStaticFields.try_emplace(fieldName, initialValue);
    } else {
      mFields.try_emplace(NameAndDescriptor{fieldName, descriptor}, std::make_unique<JField>(field, *fieldType));
    }
  }

  for (const MethodInfo& method : mClassFile->methods()) {
    types::JStringRef name = mClassFile->constantPool().getString(method.nameIndex());
    types::JStringRef descriptor = mClassFile->constantPool().getString(method.descriptorIndex());

    auto parsedDescriptor = MethodDescriptor::parse(descriptor);
    // TODO: Verification error
    assert(parsedDescriptor.has_value() && "Cannot parse descriptor");

    mMethods.try_emplace(NameAndDescriptor{name, descriptor},
                         std::make_unique<JMethod>(method, types::JString{name}, types::JString{descriptor}, *parsedDescriptor));
  }

  auto classClass = classLoader.loadClass(u"java/lang/Class");
  mClassInstance = std::make_unique<Instance>(*classClass);

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
    if (hasAccessFlag(field.accessFlags(), FieldAccessFlags::ACC_STATIC) && hasAccessFlag(field.accessFlags(), FieldAccessFlags::ACC_FINAL) &&
        field.constantValue().has_value()) {
      auto fieldName = mClassFile->constantPool().getString(field.nameIndex());
      mStaticFields.emplace(fieldName, getInitialFieldValue(field));
    }
  }

  auto clsInit = mMethods.find(NameAndDescriptor{u"<clinit>", u"()V"});
  if (clsInit != mMethods.end()) {
    vm.execute(this, clsInit->second.get());
  }

  mIsInitialized = true;
  mIsUnderInitialization = false;
}

types::JStringRef JClass::getName() const
{
  return constantPool().getClassName(mClassFile->thisClass());
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

    if (auto objectName = descriptor->asObjectName(); objectName && *objectName == u"java/lang/String") {
      return Value::Reference(mRuntimeConstantPool->getString(cvIndex));
      // TODO: Handle String types
      assert(false && "TODO strings");
    }

    return std::nullopt;
  });

  if (value) {
    return *value;
  }

  assert(false && "There should a ConstantValue attribute for a static field!");
  std::unreachable();
}

std::optional<ClassAndMethod> JClass::getMethod(const types::JString& name, const types::JString& descriptor)
{
  NameAndDescriptor pair{name, descriptor};
  if (auto it = mMethods.find(pair); it != mMethods.end()) {
    return ClassAndMethod{this, it->second.get()};
  }

  if (mSuperClass != nullptr) {
    if (auto superClassMethod = mSuperClass->getMethod(name, descriptor); superClassMethod) {
      return superClassMethod;
    }
  }

  for (JClass* superInterface : mSuperInterfaces) {
    if (auto superClassMethod = superInterface->getMethod(name, descriptor); superClassMethod) {
      return superClassMethod;
    }
  }

  // TODO: Return JvmExpected?
  return std::nullopt;
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

void JClass::initializeRuntimeConstantPool(StringHeap& stringHeap, BootstrapClassLoader& classLoader)
{
  if (mRuntimeConstantPool == nullptr) {
    mRuntimeConstantPool = std::make_unique<RuntimeConstantPool>(mClassFile->constantPool(), stringHeap, classLoader);
  }
}

bool JClass::isSubClassOf(JClass* other) const
{
  if (this == other) {
    return true;
  }

  JClass* superClass = mSuperClass;
  while (superClass != nullptr) {
    if (superClass == other) {
      return true;
    }
    superClass = superClass->mSuperClass;
  }

  return false;
}

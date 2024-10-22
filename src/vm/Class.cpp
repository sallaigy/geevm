#include "vm/Class.h"

#include "Vm.h"

#include <utility>

using namespace geevm;

ArrayClass* JClass::asArrayClass()
{
  if (mKind == Kind::Array) {
    return static_cast<ArrayClass*>(this);
  }
  return nullptr;
}

InstanceClass* JClass::asInstanceClass()
{
  if (mKind == Kind::Instance) {
    return static_cast<InstanceClass*>(this);
  }
  return nullptr;
}

InstanceClass::InstanceClass(std::unique_ptr<ClassFile> classFile)
  : JClass(Kind::Instance, types::JString{classFile->constantPool().getClassName(classFile->thisClass())}), mClassFile(std::move(classFile))
{
}

void JClass::prepare(BootstrapClassLoader& classLoader)
{
  if (mStatus >= Status::Prepared) {
    return;
  }

  if (auto arrayClass = this->asArrayClass(); arrayClass != nullptr) {
    this->linkSuperClass(u"java/lang/Object", classLoader);
    this->linkSuperInterfaces({u"java/lang/Cloneable", u"java/lang/Serializable"}, classLoader);
  } else if (auto instanceClass = this->asInstanceClass(); instanceClass != nullptr) {
    if (auto superClass = instanceClass->constantPool().getOptionalClassName(instanceClass->mClassFile->superClass())) {
      this->linkSuperClass(*superClass, classLoader);
    }
    std::vector<types::JStringRef> interfaces;
    for (types::u2 interfaceIndex : instanceClass->mClassFile->interfaces()) {
      interfaces.push_back(instanceClass->constantPool().getClassName(interfaceIndex));
    }
    this->linkSuperInterfaces(interfaces, classLoader);
    instanceClass->linkFields();
    instanceClass->prepareMethods();
  }

  auto classClass = classLoader.loadClass(u"java/lang/Class");
  mClassInstance = std::make_unique<Instance>((*classClass)->asInstanceClass());

  mStatus = Status::Prepared;
}

void JClass::linkSuperClass(types::JStringRef className, BootstrapClassLoader& classLoader)
{
  // TODO: We should not copy the name here
  auto loaded = classLoader.loadClass(types::JString{className});
  if (!loaded) {
    // TODO
    assert(false && "TODO: Handle class load failure");
    return;
  }

  mSuperClass = *loaded;
}

void JClass::linkSuperInterfaces(const std::vector<types::JStringRef>& interfaces, BootstrapClassLoader& classLoader)
{
  for (types::JStringRef interface : interfaces) {
    // TODO: We should not copy the name here
    auto loaded = classLoader.loadClass(types::JString{interface});
    if (!loaded) {
      // TODO
      assert(false && "TODO: Handle class load failure");
      return;
    }
    mSuperInterfaces.push_back(*loaded);
  }
}

void JClass::initialize(Vm& vm)
{
  if (mStatus >= Status::UnderInitialization) {
    return;
  }

  if (auto instanceClass = this->asInstanceClass(); instanceClass != nullptr) {
    instanceClass->initializeFields();

    auto clsInit = mMethods.find(NameAndDescriptor{u"<clinit>", u"()V"});
    if (clsInit != mMethods.end()) {
      vm.execute(instanceClass, clsInit->second.get());
    }
  }

  mStatus = Status::Initialized;
}

void InstanceClass::linkFields()
{
  if (this->superClass() != nullptr) {
    for (auto& [name, value] : this->superClass()->fields()) {
      mFields.try_emplace(name, std::make_unique<JField>(value->fieldInfo(), value->fieldType()));
    }
  }

  for (const FieldInfo& field : mClassFile->fields()) {
    auto fieldName = mClassFile->constantPool().getString(field.nameIndex());
    auto descriptor = mClassFile->constantPool().getString(field.descriptorIndex());
    auto fieldType = FieldType::parse(descriptor);
    assert(fieldType.has_value());

    mFields.try_emplace(NameAndDescriptor{fieldName, descriptor}, std::make_unique<JField>(field, *fieldType));

    if (hasAccessFlag(field.accessFlags(), FieldAccessFlags::ACC_STATIC)) {
      Value initialValue = Value::defaultValue(*fieldType);
      mStaticFields.try_emplace(fieldName, initialValue);
    }
  }
}

void InstanceClass::initializeFields()
{
  for (auto& [fieldName, field] : this->fields()) {
    if (hasAccessFlag(field->fieldInfo().accessFlags(), FieldAccessFlags::ACC_STATIC) &&
        hasAccessFlag(field->fieldInfo().accessFlags(), FieldAccessFlags::ACC_FINAL) && field->fieldInfo().constantValue().has_value()) {
      mStaticFields.try_emplace(fieldName.first, getInitialFieldValue(field->fieldType(), *(field->fieldInfo().constantValue())));
    }
  }
}

Value InstanceClass::getInitialFieldValue(const FieldType& fieldType, types::u2 cvIndex)
{
  if (auto primitiveType = fieldType.asPrimitive(); primitiveType.has_value()) {
    auto entry = constantPool().getEntry(cvIndex);
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

  if (auto objectName = fieldType.asObjectName(); objectName && *objectName == u"java/lang/String") {
    return Value::Reference(mRuntimeConstantPool->getString(cvIndex));
  }

  assert(false && "Unknown field type!");
}

void InstanceClass::prepareMethods()
{
  for (const MethodInfo& method : mClassFile->methods()) {
    types::JStringRef name = mClassFile->constantPool().getString(method.nameIndex());
    types::JStringRef descriptor = mClassFile->constantPool().getString(method.descriptorIndex());

    auto parsedDescriptor = MethodDescriptor::parse(descriptor);
    // TODO: Verification error
    assert(parsedDescriptor.has_value() && "Cannot parse descriptor");

    mMethods.try_emplace(NameAndDescriptor{name, descriptor},
                         std::make_unique<JMethod>(method, types::JString{name}, types::JString{descriptor}, *parsedDescriptor));
  }
}

std::optional<ClassAndMethod> JClass::getMethod(const types::JString& name, const types::JString& descriptor)
{
  NameAndDescriptor pair{name, descriptor};
  if (auto it = mMethods.find(pair); it != mMethods.end()) {
    assert(this->asInstanceClass() != nullptr);
    return ClassAndMethod{this->asInstanceClass(), it->second.get()};
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

Value JClass::getStaticField(types::JStringRef name)
{
  return mStaticFields.at(name);
}

void JClass::storeStaticField(types::JStringRef name, Value value)
{
  mStaticFields.at(name) = value;
}

void InstanceClass::initializeRuntimeConstantPool(StringHeap& stringHeap, BootstrapClassLoader& classLoader)
{
  if (mRuntimeConstantPool == nullptr) {
    mRuntimeConstantPool = std::make_unique<RuntimeConstantPool>(mClassFile->constantPool(), stringHeap, classLoader);
  }
}

#include "vm/Class.h"
#include "vm/Instance.h"

#include "Vm.h"

#include <utility>

using namespace geevm;

JClass::JClass(Kind kind, types::JString className)
  : mKind(kind), mStatus(Status::Allocated), mClassName(std::move(className))
{
}

ArrayClass* JClass::asArrayClass()
{
  if (mKind == Kind::Array) {
    return static_cast<ArrayClass*>(this);
  }
  return nullptr;
}

const ArrayClass* JClass::asArrayClass() const
{
  if (mKind == Kind::Array) {
    return static_cast<const ArrayClass*>(this);
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

const InstanceClass* JClass::asInstanceClass() const
{
  if (mKind == Kind::Instance) {
    return static_cast<const InstanceClass*>(this);
  }
  return nullptr;
}

InstanceClass::InstanceClass(std::unique_ptr<ClassFile> classFile)
  : JClass(Kind::Instance, types::JString{classFile->constantPool().getClassName(classFile->thisClass())}), mClassFile(std::move(classFile))
{
}

void JClass::prepare(BootstrapClassLoader& classLoader, JavaHeap& heap)
{
  if (mStatus >= Status::Prepared) {
    return;
  }

  if (auto arrayClass = this->asArrayClass(); arrayClass != nullptr) {
    this->linkSuperClass(u"java/lang/Object", classLoader);
    this->linkSuperInterfaces({u"java/lang/Cloneable", u"java/io/Serializable"}, classLoader);

    auto& elementType = arrayClass->elementType();
    if (auto primitiveType = elementType.asPrimitive(); primitiveType) {
      if (elementType.dimensions() != 1) {
        assert(false && "TODO");
      }
    } else if (auto objectName = elementType.asObjectName(); objectName) {
      auto loaded = classLoader.loadClass(*objectName);
      if (!loaded) {
        // FIXME
        assert(false);
      }
      arrayClass->mElementClass = *loaded;
    }
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

void JClass::initialize(JavaThread& thread)
{
  if (mStatus >= Status::UnderInitialization) {
    return;
  }

  mStatus = Status::UnderInitialization;

  if (mSuperClass != nullptr) {
    mSuperClass->initialize(thread);
  }

  for (JClass* interface : mSuperInterfaces) {
    interface->initialize(thread);
  }

  if (auto instanceClass = this->asInstanceClass(); instanceClass != nullptr) {
    instanceClass->initializeFields();

    auto clsInit = mMethods.find(NameAndDescriptor{u"<clinit>", u"()V"});
    if (clsInit != mMethods.end()) {
      thread.executeCall(clsInit->second.get(), {});
    }
  }

  // JClass* classClass = mClassInstance->getClass();
  // auto clsInstanceInit = classClass->getVirtualMethod(u"<init>", u"()V");
  // thread.executeCall(*clsInstanceInit, {});

  // mClassInstance->setFieldValue(u"name", Value::Reference(thread.heap().intern(mClassName)));

  mStatus = Status::Initialized;
}

void InstanceClass::linkFields()
{
  if (this->superClass() != nullptr) {
    for (auto& [name, value] : this->superClass()->fields()) {
      mFields.try_emplace(name, std::make_unique<JField>(value->fieldInfo(), name.first, value->fieldType()));
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
      auto jfield = std::make_unique<JField>(field, types::JString{fieldName}, *fieldType);
      mFields.insert_or_assign(NameAndDescriptor{fieldName, descriptor}, std::move(jfield));
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
                         std::make_unique<JMethod>(method, this, types::JString{name}, types::JString{descriptor}, *parsedDescriptor));
  }
}

std::optional<JMethod*> JClass::getStaticMethod(const types::JString& name, const types::JString& descriptor)
{
  return this->getMethod(name, descriptor);
}

std::optional<JMethod*> JClass::getVirtualMethod(const types::JString& name, const types::JString& descriptor)
{
  NameAndDescriptor pair{name, descriptor};
  if (auto it = mMethods.find(pair); it != mMethods.end()) {
    return it->second.get();
  }

  if (mSuperClass != nullptr) {
    if (auto superClassMethod = mSuperClass->getVirtualMethod(name, descriptor); superClassMethod) {
      return superClassMethod;
    }
  }

  for (JClass* superInterface : mSuperInterfaces) {
    if (auto superClassMethod = superInterface->getVirtualMethod(name, descriptor); superClassMethod) {
      return superClassMethod;
    }
  }

  // TODO: Return JvmExpected?
  return std::nullopt;
}

std::optional<JMethod*> JClass::getMethod(const types::JString& name, const types::JString& descriptor)
{
  NameAndDescriptor pair{name, descriptor};
  if (auto it = mMethods.find(pair); it != mMethods.end()) {
    return it->second.get();
  }

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

bool JClass::isInstanceOf(const JClass* other) const
{
  // If S is 'this' and T is 'other':

  // If S is an ordinary (nonarray) class, then:
  if (this->isClassType()) {
    // If T is a class type, then S must be the same class as T, or S must be a subclass of T;
    if (other->isClassType()) {
      auto superClass = this;
      while (superClass != nullptr) {
        if (superClass == other) {
          return true;
        }
        superClass = superClass->superClass();
      }
      return false;
    }

    // If T is an interface type, then S must implement interface T.
    if (other->isInterface()) {
      return this->hasSuperInterface(other);
    }

    return false;
  }

  // If S is an interface type, then:
  if (this->isInterface()) {
    // If T is a class type, then T must be Object.
    if (other->isClassType()) {
      return other->className() == u"java/lang/Object";
    }

    // If T is an interface type, then T must be the same interface as S or a superinterface of S.
    if (other->isInterface()) {
      return this == other || this->hasSuperInterface(other);
    }
  }

  // If S is a class representing the array type SC[], that is, an array of components of type SC, then:
  if (auto arrayClass = this->asArrayClass(); arrayClass) {
    // If T is a class type, then T must be Object.
    if (other->isClassType()) {
      return other->className() == u"java/lang/Object";
    }
    // If T is an interface type, then T must be one of the interfaces implemented by arrays (JLS §4.10.3).
    if (other->isInterface()) {
      return other->className() == u"java/lang/Cloneable" || other->className() == u"java/io/Serializable";
    }
    // If T is an array type TC[], that is, an array of components of type TC, then one of the following must be true:
    if (auto otherArray = other->asArrayClass(); otherArray) {
      auto leftElementTy = arrayClass->elementType();
      auto rightElementTy = otherArray->elementType();

      if (auto leftPrimitive = leftElementTy.asPrimitive(), rightPrimitive = rightElementTy.asPrimitive();
          leftPrimitive.has_value() && rightPrimitive.has_value()) {
        // TC and SC are the same primitive type.
        return *leftPrimitive == *rightPrimitive;
      } else if (auto leftCls = arrayClass->elementClass(), rightCls = otherArray->elementClass(); leftCls.has_value() && rightCls.has_value()) {
        // TC and SC are reference types, and type SC can be cast to TC by these run-time rules.
        return (*leftCls)->isInstanceOf(*rightCls);
      }
    }
  }

  return false;
}

bool JClass::hasSuperInterface(const JClass* other) const
{
  std::vector<JClass*> workList = this->superInterfaces();

  while (!workList.empty()) {
    JClass* interface = workList.back();
    workList.pop_back();

    if (interface == other) {
      return true;
    }
    workList.insert(workList.end(), interface->superInterfaces().begin(), interface->superInterfaces().end());
  }

  if (mSuperClass != nullptr) {
    return mSuperClass->hasSuperInterface(other);
  }

  return false;
}

bool JClass::isInterface() const
{
  if (auto instanceClass = this->asInstanceClass(); instanceClass) {
    return hasAccessFlag(instanceClass->mClassFile->accessFlags(), ClassAccessFlags::ACC_INTERFACE);
  }
  return false;
}

bool JClass::isClassType() const
{
  if (auto instanceClass = this->asInstanceClass(); instanceClass) {
    return !this->isInterface();
  }
  return false;
}

void InstanceClass::initializeRuntimeConstantPool(StringHeap& stringHeap, BootstrapClassLoader& classLoader)
{
  if (mRuntimeConstantPool == nullptr) {
    mRuntimeConstantPool = std::make_unique<RuntimeConstantPool>(mClassFile->constantPool(), stringHeap, classLoader);
  }
}

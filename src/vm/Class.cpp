#include "vm/Class.h"
#include "vm/Instance.h"

#include "Vm.h"

#include <algorithm>
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
  : JClass(Kind::Instance, types::JString{classFile->constantPool().getClassName(classFile->thisClass())}),
    mClassFile(std::move(classFile)),
    mAllocationSize(this->headerSize())
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

    FieldType elementType = arrayClass->fieldType().asArrayType()->getElementType();
    elementType.map([&]<PrimitiveType Type>() {
      arrayClass->mElementClass = nullptr;
    }, [&](const types::JString& name) {
      arrayClass->mElementClass = *classLoader.loadClass(name);
    }, [&](const ArrayType& array) {
      arrayClass->mElementClass = *classLoader.loadClass(array.className());
    });
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
  mClassInstance = heap.gc().pin(heap.allocate<ClassInstance>((*classClass)->asInstanceClass(), this));

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

  if (!this->isInterface()) {
    if (mSuperClass != nullptr) {
      mSuperClass->initialize(thread);
    }

    for (JClass* interface : mSuperInterfaces) {
      interface->initialize(thread);
    }
  }

  if (auto instanceClass = this->asInstanceClass(); instanceClass != nullptr) {
    instanceClass->initializeFields();

    auto clsInit = mMethods.find(NameAndDescriptor{u"<clinit>", u"()V"});
    if (clsInit != mMethods.end()) {
      thread.executeCall(clsInit->second.get(), {});
    }
  }

  mStatus = Status::Initialized;
}

void InstanceClass::linkFields()
{
  if (!mFields.empty()) {
    return;
  }

  size_t staticFieldOffset = 0;

  size_t currentOffset = this->headerSize();
  if (this->superClass() != nullptr) {
    for (auto& [name, field] : this->superClass()->fields()) {
      if (!field->isStatic()) {
        size_t fieldSize = field->fieldType().sizeOf();
        currentOffset = alignTo(currentOffset, fieldSize);
        mFields.try_emplace(name, std::make_unique<JField>(field->fieldInfo(), this, name.first, name.second, field->fieldType(), currentOffset));
        currentOffset += fieldSize;
      }
    }
  }

  for (const FieldInfo& field : mClassFile->fields()) {
    auto fieldName = mClassFile->constantPool().getString(field.nameIndex());
    auto descriptor = mClassFile->constantPool().getString(field.descriptorIndex());
    auto fieldType = FieldType::parse(descriptor);
    assert(fieldType.has_value());

    std::unique_ptr<JField> jfield = nullptr;
    if (hasAccessFlag(field.accessFlags(), FieldAccessFlags::ACC_STATIC)) {
      assert(mStaticFieldValues.size() == staticFieldOffset);
      jfield = std::make_unique<JField>(field, this, types::JString{fieldName}, types::JString{descriptor}, *fieldType, staticFieldOffset++);
      mStaticFieldValues.emplace_back(Value::defaultValue(*fieldType));
    } else {
      size_t fieldSize = fieldType->sizeOf();
      currentOffset = alignTo(currentOffset, fieldSize);
      jfield = std::make_unique<JField>(field, this, types::JString{fieldName}, types::JString{descriptor}, *fieldType, currentOffset);
      currentOffset += fieldSize;
    }

    NameAndDescriptor key{fieldName, descriptor};
    mFields.try_emplace(key, std::move(jfield));
  }

  mAllocationSize = currentOffset;
}

void InstanceClass::initializeFields()
{
  for (auto& [fieldName, field] : this->fields()) {
    if (field->isStatic() && field->isFinal() && field->fieldInfo().constantValue().has_value()) {
      mStaticFieldValues.at(field->offset()) = getInitialFieldValue(field->fieldType(), *(field->fieldInfo().constantValue()));
    }
  }
}

Value InstanceClass::getInitialFieldValue(const FieldType& fieldType, types::u2 cvIndex)
{
  if (auto primitiveType = fieldType.asPrimitive(); primitiveType.has_value()) {
    auto entry = constantPool().getEntry(cvIndex);
    switch (*primitiveType) {
      case PrimitiveType::Int: return Value::from(entry.data.singleInteger);
      case PrimitiveType::Char: return Value::from(static_cast<char16_t>(entry.data.singleInteger));
      case PrimitiveType::Byte: return Value::from(static_cast<int8_t>(entry.data.singleInteger));
      case PrimitiveType::Short: return Value::from(static_cast<int16_t>(entry.data.singleInteger));
      case PrimitiveType::Boolean: return Value::from(entry.data.singleInteger);
      case PrimitiveType::Float: return Value::from(entry.data.singleFloat);
      case PrimitiveType::Long: return Value::from(entry.data.doubleInteger);
      case PrimitiveType::Double: return Value::from(entry.data.doubleFloat);
    }
    // No other primitive types are possible
    GEEVM_UNREACHBLE("Unknown primitive type");
  }

  if (auto objectName = fieldType.asReference(); objectName && *objectName == u"java/lang/String") {
    return Value::from(mRuntimeConstantPool->getString(cvIndex));
  }

  GEEVM_UNREACHBLE("Unknown field type");
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

std::optional<JField*> JClass::lookupField(const types::JString& name, const types::JString& descriptor)
{
  NameAndDescriptor pair{name, descriptor};
  if (auto it = mFields.find(pair); it != mFields.end()) {
    return it->second.get();
  }

  for (JClass* superInterface : this->superInterfaces()) {
    auto result = superInterface->lookupField(name, descriptor);
    if (result) {
      return result;
    }
  }

  if (mSuperClass != nullptr) {
    return mSuperClass->lookupField(name, descriptor);
  }

  return std::nullopt;
}

std::optional<JField*> JClass::lookupFieldByName(const types::JString& string)
{
  for (auto& [key, field] : mFields) {
    if (key.first == string) {
      return field.get();
    }
  }

  return std::nullopt;
}

Value JClass::getStaticFieldValue(const types::JString& name, const types::JString& descriptor)
{
  NameAndDescriptor pair{name, descriptor};
  size_t offset = mFields.at(pair)->offset();

  return getStaticFieldValue(offset);
}

Value JClass::getStaticFieldValue(size_t offset)
{
  assert(offset < mStaticFieldValues.size());
  return mStaticFieldValues[offset];
}

void JClass::setStaticFieldValue(const types::JString& name, const types::JString& descriptor, Value value)
{
  NameAndDescriptor pair{name, descriptor};
  size_t offset = mFields.at(pair)->offset();

  this->setStaticFieldValue(offset, value);
}

void JClass::setStaticFieldValue(size_t offset, Value value)
{
  assert(offset < mStaticFieldValues.size());
  mStaticFieldValues[offset] = value;
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
      auto leftElementTy = arrayClass->fieldType().asArrayType()->getElementType();
      auto rightElementTy = otherArray->fieldType().asArrayType()->getElementType();

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

types::JString JClass::javaClassName() const
{
  if (auto instanceClass = this->asInstanceClass(); instanceClass) {
    types::JString name = this->className();
    std::ranges::replace(name, u'/', u'.');

    return name;
  }

  if (auto arrayClass = this->asArrayClass(); arrayClass) {
    return arrayClass->fieldType().toJavaString();
  }

  GEEVM_UNREACHBLE("A class must be an instance or an array!");
}

size_t JClass::headerSize() const
{
  if (auto instanceClass = this->asInstanceClass(); instanceClass) {
    if (instanceClass->className() == u"java/lang/Class") {
      return sizeof(ClassInstance);
    }
    return sizeof(Instance);
  }

  if (auto arrayClass = this->asArrayClass(); arrayClass) {
    return sizeof(ArrayInstance);
  }

  GEEVM_UNREACHBLE("A class must be an instance or an array!");
}

std::size_t JClass::alignment() const
{
  if (auto instanceClass = this->asInstanceClass(); instanceClass) {
    if (instanceClass->className() == u"java/lang/Class") {
      return alignof(ClassInstance);
    }
    return alignof(Instance);
  }

  if (auto arrayClass = this->asArrayClass(); arrayClass) {
    return alignof(ArrayInstance);
  }

  GEEVM_UNREACHBLE("A class must be an instance or an array!");
}

void InstanceClass::initializeRuntimeConstantPool(StringHeap& stringHeap, BootstrapClassLoader& classLoader)
{
  if (mRuntimeConstantPool == nullptr) {
    mRuntimeConstantPool = std::make_unique<RuntimeConstantPool>(mClassFile->constantPool(), stringHeap, classLoader);
  }
}

GcRootRef<ClassInstance> JClass::classInstance() const
{
  return mClassInstance;
}

std::size_t ArrayClass::allocationSize(int32_t length) const
{
  assert(length >= 0);
  auto elementType = this->fieldType().asArrayType()->getElementType();
  return this->headerSize() + length * elementType.sizeOf();
}

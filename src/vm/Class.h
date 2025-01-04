#ifndef GEEVM_VM_CLASS_H
#define GEEVM_VM_CLASS_H

#include "Frame.h"
#include "Thread.h"

#include <unordered_map>

#include "class_file/ClassFile.h"
#include "common/Hash.h"
#include "common/JvmTypes.h"
#include "vm/Field.h"
#include "vm/Instance.h"
#include "vm/Method.h"
#include "vm/Runtime.h"

namespace geevm
{
class Vm;
class StringHeap;
class InstanceClass;
class ClassInstance;
class ArrayClass;
class JMethod;
class JavaHeap;

// using ClassAndMethod = std::pair<JClass*,JMethod*>;

struct ClassAndMethod
{
  InstanceClass* const klass;
  JMethod* const method;
};

class JClass
{
  friend class Vm;
  friend class BootstrapClassLoader;

public:
  enum class Kind
  {
    Instance,
    Array
  };

  enum class Status
  {
    Allocated,
    Loaded,
    Prepared,
    UnderInitialization,
    Initialized
  };

protected:
  JClass(Kind kind, types::JString className);

public:
  JClass(const JClass&) = delete;
  JClass& operator=(const JClass&) = delete;

  virtual ~JClass() = default;

  // Basic class metadata
  //==----------------------------------------------------------------------==//
  const types::JString& className() const
  {
    return mClassName;
  }

  types::JString javaClassName() const;

  JClass* superClass() const
  {
    return mSuperClass;
  }

  const std::vector<JClass*>& superInterfaces() const
  {
    return mSuperInterfaces;
  }

  Instance* classInstance() const;
  void setClassInstance(ClassInstance* instance);

  // Methods and fields
  //==----------------------------------------------------------------------==//
  std::optional<JMethod*> getMethod(const types::JString& name, const types::JString& descriptor);

  std::optional<JMethod*> getStaticMethod(const types::JString& name, const types::JString& descriptor);
  std::optional<JMethod*> getVirtualMethod(const types::JString& name, const types::JString& descriptor);

  Value getStaticFieldValue(const types::JString& name, const types::JString& descriptor);
  Value getStaticFieldValue(size_t offset);

  template<JvmType T>
  T getStaticFieldValue(size_t offset)
  {
    return getStaticFieldValue(offset).get<T>();
  }

  template<JvmType T>
  T getStaticFieldValue(const types::JString& name, const types::JString& descriptor)
  {
    return getStaticFieldValue(name, descriptor).get<T>();
  }

  void setStaticFieldValue(const types::JString& name, const types::JString& descriptor, Value value);
  void setStaticFieldValue(size_t offset, Value value);

  template<JvmType T>
  void setStaticFieldValue(size_t offset, T value)
  {
    this->setStaticFieldValue(offset, Value::from<T>(value));
  }

  std::optional<JField*> lookupField(const types::JString& name, const types::JString& descriptor);
  std::optional<JField*> lookupFieldByName(const types::JString& string);

  const std::unordered_map<NameAndDescriptor, std::unique_ptr<JField>, PairHash>& fields() const
  {
    return mFields;
  }

  size_t numInstanceFields() const
  {
    // TODO: This should be a constant operation
    size_t count = 0;
    for (auto& [key, field] : mFields) {
      if (!field->isStatic()) {
        count++;
      }
    }
    return count;
  }

  // Static polymorphism
  //==----------------------------------------------------------------------==//
  InstanceClass* asInstanceClass();
  const InstanceClass* asInstanceClass() const;
  ArrayClass* asArrayClass();
  const ArrayClass* asArrayClass() const;

  // Linking and initialization
  //==----------------------------------------------------------------------==//
  void prepare(BootstrapClassLoader& classLoader, JavaHeap& heap);
  void initialize(JavaThread& thread);

  // Query methods
  //==----------------------------------------------------------------------==//

  /// Returns true iff this class represent an ordinary (nonarray) class that is not an interface.
  bool isClassType() const;
  bool isArrayType() const
  {
    return mKind == Kind::Array;
  }
  bool isInterface() const;

  bool isInstanceOf(const JClass* other) const;
  bool hasSuperInterface(const JClass* other) const;

  bool isInitialized() const
  {
    return mStatus >= Status::Initialized;
  }

  bool isUnderInitialization() const
  {
    return mStatus == Status::UnderInitialization;
  }

  size_t headerSize() const;

private:
  void linkSuperClass(types::JStringRef className, BootstrapClassLoader& classLoader);
  void linkSuperInterfaces(const std::vector<types::JStringRef>& interfaces, BootstrapClassLoader& classLoader);

protected:
  const Kind mKind;
  Status mStatus;

  std::unordered_map<NameAndDescriptor, std::unique_ptr<JMethod>, PairHash> mMethods;
  std::unordered_map<NameAndDescriptor, std::unique_ptr<JField>, PairHash> mFields;
  std::vector<JField*> mInstanceFields;
  std::vector<Value> mStaticFieldValues;

private:
  types::JString mClassName;
  JClass* mSuperClass = nullptr;
  std::vector<JClass*> mSuperInterfaces;

  ClassInstance* mClassInstance = nullptr;
};

class InstanceClass : public JClass
{
  friend class Vm;
  friend class BootstrapClassLoader;
  friend class JClass;

public:
  explicit InstanceClass(std::unique_ptr<ClassFile> classFile);

  const ConstantPool& constantPool() const
  {
    return mClassFile->constantPool();
  }

  RuntimeConstantPool& runtimeConstantPool() const
  {
    return *mRuntimeConstantPool;
  }

  bool isSubClassOf(InstanceClass* other) const;

  size_t allocationSize() const
  {
    return mAllocationSize;
  }

private:
  void initializeRuntimeConstantPool(StringHeap& stringHeap, BootstrapClassLoader& classLoader);
  Value getInitialFieldValue(const FieldType& fieldType, types::u2 cvIndex);
  void prepareMethods();

protected:
  void linkFields();
  void initializeFields();

private:
  std::unique_ptr<ClassFile> mClassFile;
  std::unique_ptr<RuntimeConstantPool> mRuntimeConstantPool;
  size_t mAllocationSize;
};

class ArrayClass : public JClass
{
  friend class JClass;

public:
  explicit ArrayClass(types::JString className, FieldType type)
    : JClass(Kind::Array, std::move(className)), mType(std::move(type))
  {
  }

  const FieldType& fieldType() const
  {
    return mType;
  }

  std::optional<JClass*> elementClass() const
  {
    return mElementClass;
  }

  std::size_t allocationSize(int32_t length) const;

private:
  FieldType mType;
  std::optional<JClass*> mElementClass = std::nullopt;
};

} // namespace geevm

#endif // GEEVM_VM_CLASS_H

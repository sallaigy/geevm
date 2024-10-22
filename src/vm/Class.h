#ifndef GEEVM_VM_CLASS_H
#define GEEVM_VM_CLASS_H

#include "Frame.h"

#include <unordered_map>

#include "class_file/ClassFile.h"
#include "common/Hash.h"
#include "common/JvmTypes.h"
#include "vm/Field.h"
#include "vm/Method.h"
#include "vm/Runtime.h"

namespace geevm
{
class Vm;
class StringHeap;
class InstanceClass;
class ArrayClass;
class JMethod;

// using ClassAndMethod = std::pair<JClass*,JMethod*>;

struct ClassAndMethod
{
  InstanceClass* const klass;
  JMethod* const method;
};

class JClass
{
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
  JClass(Kind kind, types::JString className)
    : mKind(kind), mStatus(Status::Allocated), mClassName(std::move(className))
  {
  }

public:
  // Basic class metadata
  //==----------------------------------------------------------------------==//

  const types::JString& className() const
  {
    return mClassName;
  }

  JClass* superClass() const
  {
    return mSuperClass;
  }

  const std::vector<JClass*>& superInterfaces() const
  {
    return mSuperInterfaces;
  }

  Instance* classInstance() const
  {
    return mClassInstance.get();
  }

  // Methods and fields
  //==----------------------------------------------------------------------==//
  std::optional<ClassAndMethod> getMethod(const types::JString& name, const types::JString& descriptor);

  Value getStaticField(types::JStringRef name);
  void storeStaticField(types::JStringRef name, Value);

  const std::unordered_map<NameAndDescriptor, std::unique_ptr<JField>, PairHash>& fields() const
  {
    return mFields;
  }

  // Static polymorphism
  //==----------------------------------------------------------------------==//
  InstanceClass* asInstanceClass();
  ArrayClass* asArrayClass();

  // Linking and initialization
  //==----------------------------------------------------------------------==//
  void prepare(BootstrapClassLoader& classLoader);
  void initialize(Vm& vm);

private:
  void linkSuperClass(types::JStringRef className, BootstrapClassLoader& classLoader);
  void linkSuperInterfaces(const std::vector<types::JStringRef>& interfaces, BootstrapClassLoader& classLoader);

protected:
  const Kind mKind;
  Status mStatus;

protected:
  std::unordered_map<NameAndDescriptor, std::unique_ptr<JMethod>, PairHash> mMethods;
  std::unordered_map<NameAndDescriptor, std::unique_ptr<JField>, PairHash> mFields;
  std::unordered_map<types::JStringRef, Value> mStaticFields;

private:
  types::JString mClassName;
  JClass* mSuperClass = nullptr;
  std::vector<JClass*> mSuperInterfaces;

  std::unique_ptr<Instance> mClassInstance;
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
};

class ArrayClass : public JClass
{
public:
  explicit ArrayClass(types::JString className, FieldType type)
    : JClass(Kind::Array, std::move(className)), mType(std::move(type))
  {
    // FIXME
  }

  const FieldType& getType() const
  {
    return mType;
  }

private:
  FieldType mType;
};

} // namespace geevm

#endif // GEEVM_VM_CLASS_H

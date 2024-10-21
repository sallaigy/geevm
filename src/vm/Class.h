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
class JClass;
class JMethod;

// using ClassAndMethod = std::pair<JClass*,JMethod*>;

struct ClassAndMethod
{
  JClass* const klass;
  JMethod* const method;
};

class JClass
{
  friend class Vm;
  friend class BootstrapClassLoader;

  enum class Kind
  {
    Instance,
    Array
  };

public:
  explicit JClass(std::unique_ptr<ClassFile> classFile);

  void prepare(BootstrapClassLoader& classLoader);
  void initialize(Vm& vm);

  types::JStringRef getName() const;

  std::optional<ClassAndMethod> getMethod(const types::JString& name, const types::JString& descriptor);

  std::optional<types::JStringRef> superClass() const;
  std::vector<types::JStringRef> interfaces() const;

  Value getStaticField(types::JStringRef name);
  void storeStaticField(types::JStringRef name, Value);

  const std::unordered_map<NameAndDescriptor, std::unique_ptr<JField>, PairHash>& fields() const
  {
    return mFields;
  }

  const ConstantPool& constantPool() const
  {
    return mClassFile->constantPool();
  }

  RuntimeConstantPool& runtimeConstantPool() const
  {
    return *mRuntimeConstantPool;
  }

  const types::JString& className() const
  {
    return mClassName;
  }

  Instance* classInstance() const
  {
    return mClassInstance.get();
  }

  bool isSubClassOf(JClass* other) const;

private:
  void initializeRuntimeConstantPool(StringHeap& stringHeap, BootstrapClassLoader& classLoader);
  Value getInitialFieldValue(const FieldInfo& field);

  bool mIsPrepared = false;
  bool mIsInitialized = false;
  bool mIsUnderInitialization = false;
  std::unique_ptr<ClassFile> mClassFile;

  types::JString mClassName;
  JClass* mSuperClass = nullptr;
  std::vector<JClass*> mSuperInterfaces;

  std::unique_ptr<RuntimeConstantPool> mRuntimeConstantPool;
  std::unordered_map<NameAndDescriptor, std::unique_ptr<JMethod>, PairHash> mMethods;
  std::unordered_map<NameAndDescriptor, std::unique_ptr<JField>, PairHash> mFields;
  std::unordered_map<types::JStringRef, Value> mStaticFields;
  std::unique_ptr<Instance> mClassInstance;
};

class ArrayClass : public JClass
{
public:
  explicit ArrayClass(FieldType type)
    : JClass(nullptr), mType(std::move(type))
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

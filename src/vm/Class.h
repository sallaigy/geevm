#ifndef GEEVM_VM_CLASS_H
#define GEEVM_VM_CLASS_H

#include "Frame.h"

#include <unordered_map>

#include "class_file/ClassFile.h"
#include "common/Hash.h"
#include "common/JvmTypes.h"
#include "vm/Field.h"
#include "vm/Method.h"

namespace geevm
{

class Vm;

class JClass
{
  friend class Vm;

public:
  explicit JClass(std::unique_ptr<ClassFile> classFile);

  void prepare();
  void initialize(Vm& vm);

  MethodRef getMethodRef(types::u2 index);
  const FieldRef& getFieldRef(types::u2 index);

  JMethod* getMethod(const types::JString& name, const types::JString& descriptor);

  Value getStaticField(types::JStringRef name);
  void storeStaticField(types::JStringRef name, Value);

  const ConstantPool& constantPool() const
  {
    return mClassFile->constantPool();
  }

  std::optional<types::JStringRef> superClass() const;
  std::vector<types::JStringRef> interfaces() const;

private:
  Value getInitialFieldValue(const FieldInfo& field);

  bool mIsPrepared = false;
  bool mIsInitialized = false;
  bool mIsUnderInitialization = false;
  std::unique_ptr<ClassFile> mClassFile;
  std::unordered_map<types::u2, MethodRef> mMethodRefCache;
  std::unordered_map<types::u2, FieldRef> mFieldRefCache;
  std::unordered_map<NameAndDescriptor, std::unique_ptr<JMethod>, PairHash> mMethods;
  std::unordered_map<NameAndDescriptor, std::unique_ptr<JField>, PairHash> mFields;
  std::unordered_map<types::JStringRef, Value> mStaticFields;
};

} // namespace geevm

#endif // GEEVM_VM_CLASS_H

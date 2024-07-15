#ifndef GEEVM_VM_CLASS_H
#define GEEVM_VM_CLASS_H

#include "common/JvmTypes.h"

#include "vm/Method.h"
#include "class_file/ClassFile.h"

#include <unordered_map>

namespace geevm
{

class JClass
{
  friend class Vm;
  using MethodNameAndDescriptor = std::pair<types::JString, types::JString>;

  struct PairHash {
    std::size_t operator()(const MethodNameAndDescriptor& pair) const
    {
      std::size_t hash = 17;
      hash = hash * 31 + std::hash<types::JString>()(pair.first);
      return hash * 31 + std::hash<types::JString>()(pair.second);
    }
  };
public:
  explicit JClass(std::unique_ptr<ClassFile> classFile)
    : mClassFile(std::move(classFile))
  {}

  MethodRef getMethodRef(types::u2 index);
  JMethod* getMethod(const types::JString& name, const types::JString& descriptor);

  const ConstantPool& constantPool() const
  {
    return mClassFile->constantPool();
  }

private:
  std::unique_ptr<ClassFile> mClassFile;
  std::unordered_map<MethodNameAndDescriptor, std::unique_ptr<JMethod>, PairHash> mMethods;
};

}

#endif //GEEVM_VM_CLASS_H

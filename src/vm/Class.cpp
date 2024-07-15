#include "vm/Class.h"

using namespace geevm;

JMethod* JClass::getMethod(const types::JString& name, const types::JString& descriptor)
{
  MethodNameAndDescriptor pair{name, descriptor};
  if (auto it = mMethods.find(pair); it != mMethods.end()) {
    return it->second.get();
  }

  for (auto& method : mClassFile->methods()) {
    auto methodName = mClassFile->constantPool().getString(method.nameIndex());
    auto descriptorString = mClassFile->constantPool().getString(method.descriptorIndex());
    if (methodName == name && descriptorString == descriptor) {
      auto r = mMethods.emplace(pair, std::make_unique<JMethod>(method));
      return r.first->second.get();
    }
  }

  // TODO: Return JvmExpected?
  return nullptr;
}

MethodRef JClass::getMethodRef(types::u2 index)
{
  auto& entry = mClassFile->constantPool().getEntry(index);
  assert(entry.tag == ConstantPool::Tag::CONSTANT_Methodref && "Can only fetch a method ref from a method ref entry!");

  types::JStringRef className = mClassFile->constantPool().getClassName(entry.data.classAndNameRef.classIndex);
  auto [methodName, descriptor] = mClassFile->constantPool().getNameAndType(entry.data.classAndNameRef.nameAndTypeIndex);

  return MethodRef{className, methodName, descriptor};
}

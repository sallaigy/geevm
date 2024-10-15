#include "vm/Runtime.h"

#include "ClassLoader.h"
#include "StringHeap.h"
#include "vm/Field.h"
#include "vm/Method.h"

#include <common/JvmError.h>

using namespace geevm;

const MethodRef& RuntimeConstantPool::getMethodRef(types::u2 index)
{
  if (auto it = mMethodRefs.find(index); it != mMethodRefs.end()) {
    return it->second;
  }

  auto& entry = mConstantPool.getEntry(index);
  assert(entry.tag == ConstantPool::Tag::CONSTANT_Methodref && "Can only fetch a method ref from a method ref entry!");

  types::JString className{mConstantPool.getClassName(entry.data.classAndNameRef.classIndex)};
  auto [methodName, descriptor] = mConstantPool.getNameAndType(entry.data.classAndNameRef.nameAndTypeIndex);

  auto [it, _] = mMethodRefs.try_emplace(index, className, types::JString{methodName}, types::JString{descriptor});
  return it->second;
}

const FieldRef& RuntimeConstantPool::getFieldRef(types::u2 index)
{
  if (auto it = mFieldRefs.find(index); it != mFieldRefs.end()) {
    return it->second;
  }

  auto& entry = mConstantPool.getEntry(index);
  assert(entry.tag == ConstantPool::Tag::CONSTANT_Fieldref && "Can only fetch a method ref from a method ref entry!");

  types::JString className{mConstantPool.getClassName(entry.data.classAndNameRef.classIndex)};
  auto [fieldName, descriptor] = mConstantPool.getNameAndType(entry.data.classAndNameRef.nameAndTypeIndex);

  auto [it, _] = mFieldRefs.try_emplace(index, className, types::JString{fieldName}, types::JString{descriptor});
  return it->second;
}

Instance* RuntimeConstantPool::getString(types::u2 index)
{
  if (auto it = mStrings.find(index); it != mStrings.end()) {
    return it->second;
  }

  auto& entry = mConstantPool.getEntry(index);
  assert(entry.tag == ConstantPool::Tag::CONSTANT_String);

  types::JStringRef utf8 = mConstantPool.getString(entry.data.stringInfo.stringIndex);
  auto [res, _] = mStrings.try_emplace(index, mStringHeap.intern(utf8));

  return res->second;
}

JvmExpected<JClass*> RuntimeConstantPool::getClass(types::u2 index)
{
  if (auto it = mClasses.find(index); it != mClasses.end()) {
    return it->second;
  }

  auto className = mConstantPool.getClassName(index);
  auto klass = mBootstrapClassLoader.loadClass(types::JString{className});

  if (!klass) {
    return klass;
  }

  auto [res, _] = mClasses.try_emplace(index, *klass);
  return res->second;
}

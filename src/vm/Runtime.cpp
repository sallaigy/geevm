#include "vm/Runtime.h"

#include "ClassLoader.h"
#include "StringHeap.h"
#include "vm/Field.h"
#include "vm/Method.h"

#include <common/JvmError.h>

using namespace geevm;

JMethod* RuntimeConstantPool::getMethodRef(types::u2 index)
{
  if (auto it = mMethodRefs.find(index); it != mMethodRefs.end()) {
    return it->second;
  }

  auto& entry = mConstantPool.getEntry(index);
  assert((entry.tag == ConstantPool::Tag::CONSTANT_Methodref || entry.tag == ConstantPool::Tag::CONSTANT_InterfaceMethodref) &&
         "Can only fetch a method ref from a method ref entry!");

  types::JString className{mConstantPool.getClassName(entry.data.classAndNameRef.classIndex)};
  auto [methodName, descriptor] = mConstantPool.getNameAndType(entry.data.classAndNameRef.nameAndTypeIndex);

  auto klass = mBootstrapClassLoader.loadClass(className);
  if (!klass) {
    // TODO
    assert(false && "Should resolve class in getMethodRef");
  }

  auto method = (*klass)->getVirtualMethod(types::JString{methodName}, types::JString{descriptor});
  if (!method.has_value()) {
    // TODO
    assert(false);
  }

  auto [it, _] = mMethodRefs.try_emplace(index, *method);
  return it->second;
}

const FieldRef& RuntimeConstantPool::getFieldRef(types::u2 index)
{
  if (auto it = mFieldRefs.find(index); it != mFieldRefs.end()) {
    return it->second;
  }

  auto& entry = mConstantPool.getEntry(index);
  assert(entry.tag == ConstantPool::Tag::CONSTANT_Fieldref && "Can only fetch a method ref from a method ref entry!");

  auto klass = this->getClass(entry.data.classAndNameRef.classIndex);
  assert(klass.has_value() && "TODO: Return error if resolution fails.");
  auto [fieldName, descriptor] = mConstantPool.getNameAndType(entry.data.classAndNameRef.nameAndTypeIndex);

  auto [it, _] = mFieldRefs.try_emplace(index, (*klass), types::JString{fieldName}, types::JString{descriptor});
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

#include "class_file/ConstantPool.h"

using namespace geevm;

types::JStringRef ConstantPool::getClassName(types::u2 index) const
{
  const Entry& entry = this->getEntry(index);
  assert(entry.tag == Tag::CONSTANT_Class && "Can only fetch a class name from a class entry!");

  return getString(entry.data.classInfo.nameIndex);
}

std::optional<types::JStringRef> ConstantPool::getOptionalClassName(types::u2 index) const
{
  if (index == 0) {
    return std::nullopt;
  }
  return this->getClassName(index);
}

types::JStringRef ConstantPool::getString(types::u2 index) const
{
  const Entry& entry = this->getEntry(index);
  assert(entry.tag == Tag::CONSTANT_Utf8 && "Can only fetch a string from a Utf8 entry!");

  return mStrings.at(entry.data.utf8String);
}

std::pair<types::JStringRef, types::JStringRef> ConstantPool::getNameAndType(types::u2 index) const
{
  const Entry& entry = this->getEntry(index);
  assert(entry.tag == Tag::CONSTANT_NameAndType && "Can only fetch a name and type from a NameAndType entry!");

  return {getString(entry.data.nameAndType.nameIndex), getString(entry.data.nameAndType.descriptorIndex)};
}

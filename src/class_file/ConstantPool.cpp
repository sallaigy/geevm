#include "class_file/ConstantPool.h"

using namespace geevm;

std::u16string_view ConstantPool::getClassName(types::u2 index) const
{
  const Entry& entry = this->getEntry(index);
  assert(entry.tag == Tag::CONSTANT_Class && "Can only fetch a class name from a class entry!");

  return getString(entry.data.classInfo.nameIndex);
}

std::u16string_view ConstantPool::getString(types::u2 index) const
{
  const Entry& entry = this->getEntry(index);
  assert(entry.tag == Tag::CONSTANT_Utf8 && "Can only fetch a string from a Utf8 entry!");

  return mStrings.at(entry.data.utf8String);
}

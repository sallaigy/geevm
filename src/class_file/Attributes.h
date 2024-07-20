#ifndef GEEVM_CLASS_FILE_ATTRIBUTES_H
#define GEEVM_CLASS_FILE_ATTRIBUTES_H

#include "common/JvmTypes.h"
#include <vector>

namespace geevm
{

class Code
{
public:
  struct ExceptionTableEntry
  {
    types::u2 startPc;
    types::u2 endPc;
    types::u2 handlerPc;
    types::u2 catchType;
  };

  struct LocalVariableTableEntry
  {
    types::u2 startPc;
    types::u2 length;
    types::u2 nameIndex;
    types::u2 descriptorIndex;
    types::u2 index;
  };

  // Constructors
  //==--------------------------------------------------------------------==//
  Code(types::u2 maxStack, types::u2 maxLocals, std::vector<types::u1> bytes, std::vector<ExceptionTableEntry> exceptionTable,
       std::vector<LocalVariableTableEntry> localVariableTable, std::vector<LocalVariableTableEntry> localVariableTypeTable)
    : mMaxStack{maxStack},
      mMaxLocals{maxLocals},
      mBytes{std::move(bytes)},
      mExceptionTable{std::move(exceptionTable)},
      mLocalVariableTable{std::move(localVariableTable)},
      mLocalVariableTypeTable{std::move(localVariableTypeTable)}
  {
  }

  Code(const Code&) = delete;
  Code(Code&&) = default;

  types::u2 maxStack() const
  {
    return mMaxStack;
  }

  types::u2 maxLocals() const
  {
    return mMaxLocals;
  }

  const std::vector<types::u1>& bytes() const
  {
    return mBytes;
  }

  const std::vector<ExceptionTableEntry>& exceptionTable() const
  {
    return mExceptionTable;
  }

  const std::vector<LocalVariableTableEntry>& localVariableTable() const
  {
    return mLocalVariableTable;
  }

  const std::vector<LocalVariableTableEntry>& localVariableTypeTable() const
  {
    return mLocalVariableTypeTable;
  }

private:
  types::u2 mMaxStack;
  types::u2 mMaxLocals;
  std::vector<types::u1> mBytes;
  std::vector<ExceptionTableEntry> mExceptionTable;
  std::vector<LocalVariableTableEntry> mLocalVariableTable;
  std::vector<LocalVariableTableEntry> mLocalVariableTypeTable;
};

} // namespace geevm

#endif // GEEVM_CLASS_FILE_ATTRIBUTES_H

#ifndef GEEVM_VM_CODE_H
#define GEEVM_VM_CODE_H

#include <vector>

#include "class_file/Opcode.h"
#include "common/JvmTypes.h"

namespace geevm
{

// TODO: Move "Code" here?

class CodeCursor
{
public:
  explicit CodeCursor(const std::vector<types::u1>& code, int64_t startPos = 0)
    : mCode(code), mPos(startPos)
  {
  }

  bool hasNext()
  {
    return mPos < mCode.size();
  }

  int64_t position() const
  {
    return mPos;
  }

  void set(int64_t target)
  {
    // TODO: Check bounds
    mPos = target;
  }

  Opcode next()
  {
    return static_cast<Opcode>(mCode[mPos++]);
  }

  types::u1 readU1()
  {
    return mCode[mPos++];
  }

  types::u2 readU2()
  {
    types::u2 value = (mCode[mPos] << 8u) | mCode[mPos + 1];
    mPos += 2;
    return value;
  }

  types::u4 readU4();

private:
  const std::vector<types::u1>& mCode;
  int64_t mPos;
};

} // namespace geevm

#endif // GEEVM_VM_CODE_H

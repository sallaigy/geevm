#include "class_file/Code.h"

using namespace geevm;

bool CodeCursor::hasNext()
{
  return mPos < mCode.size();
}

Opcode CodeCursor::next()
{
  return static_cast<Opcode>(mCode[mPos++]);
}

types::u1 CodeCursor::readU1()
{
  return mCode[mPos++];
}

types::u2 CodeCursor::readU2()
{
  types::u2 value = (mCode[mPos] << 8u) | mCode[mPos + 1];
  mPos += 2;
  return value;
}

types::u4 CodeCursor::readU4()
{
  types::u4 value = (mCode[mPos] << 24u) | (mCode[mPos + 1] << 16u) | (mCode[mPos + 2] << 8u) | mCode[mPos + 3];
  mPos += 4;
  return value;
}

std::string geevm::opcodeToString(geevm::Opcode opcode)
{
#define GEEVM_HANDLE_OPCODE(MNEMONIC, OPCODE) \
  case Opcode::MNEMONIC: return #MNEMONIC;

  switch (opcode) {
#include "class_file/Opcode.def"
  }
#undef GEEVM_HANDLE_OPCODE

  return "UNKNOWN";
}

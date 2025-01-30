#include "class_file/Code.h"

using namespace geevm;

types::u4 CodeCursor::readU4()
{
  types::u4 value = (mCode[mPos] << 24u) | (mCode[mPos + 1] << 16u) | (mCode[mPos + 2] << 8u) | mCode[mPos + 3];
  mPos += 4;
  return value;
}

std::string geevm::opcodeToString(Opcode opcode)
{
#define GEEVM_HANDLE_OPCODE(MNEMONIC, OPCODE) \
  case Opcode::MNEMONIC: return #MNEMONIC;

  switch (opcode) {
#include "class_file/Opcode.def"
  }
#undef GEEVM_HANDLE_OPCODE

  return "UNKNOWN";
}

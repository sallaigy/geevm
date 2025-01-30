#include "class_file/Opcode.h"

using namespace geevm;

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

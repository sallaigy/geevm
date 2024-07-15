#ifndef GEEVM_CLASS_FILE_OPCODE_H
#define GEEVM_CLASS_FILE_OPCODE_H

namespace geevm
{

enum class Opcode
{
#define GEEVM_HANDLE_OPCODE(MNEMONIC, OPCODE) MNEMONIC = OPCODE,
#include "Opcode.def"

#undef GEEVM_HANDLE_OPCODE
};

std::string opcodeToString(Opcode opcode);

}

#endif //GEEVM_CLASS_FILE_OPCODE_H

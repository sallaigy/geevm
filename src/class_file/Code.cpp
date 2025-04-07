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

size_t geevm::bytesConsumedByOpcode(Opcode opcode)
{
  switch (opcode) {
    using enum Opcode;
    case BIPUSH: return 1;
    case SIPUSH: return 2;
    case LDC: return 1;
    case LDC_W:
    case LDC2_W: return 2;
    case ILOAD:
    case LLOAD:
    case FLOAD:
    case DLOAD:
    case ALOAD: return 1;
    case ISTORE:
    case LSTORE:
    case FSTORE:
    case DSTORE:
    case ASTORE: return 1;
    case IINC: return 2;
    case IFEQ:
    case IFNE:
    case IFLT:
    case IFGE:
    case IFGT:
    case IFLE:
    case IF_ICMPEQ:
    case IF_ICMPNE:
    case IF_ICMPLT:
    case IF_ICMPGE:
    case IF_ICMPGT:
    case IF_ICMPLE:
    case IF_ACMPEQ:
    case IF_ACMPNE:
    case GOTO:
    case GETSTATIC:
    case PUTSTATIC:
    case GETFIELD:
    case PUTFIELD:
    case INVOKEVIRTUAL:
    case INVOKESPECIAL:
    case INVOKESTATIC: return 2;
    case INVOKEINTERFACE: return 4;
    case NEW: return 2;
    case NEWARRAY: return 1;
    case ANEWARRAY: return 2;
    case CHECKCAST: return 2;
    case INSTANCEOF: return 2;
    case WIDE: return 3;
    case MULTIANEWARRAY: return 3;
    case IFNULL:
    case IFNONNULL: return 2;
    default: return 0;
  }
}
#include "vm/GcRoots.h"

#include "Class.h"
#include "common/ByteStream.h"
#include "vm/Frame.h"
#include "vm/Thread.h"

using namespace geevm;

namespace
{

size_t bytesConsumedByOpcode(Opcode opcode);

bool isReference(VerificationTypeInfo vti)
{
  switch (vti) {
    case VerificationTypeInfo::Top:
    case VerificationTypeInfo::Integer:
    case VerificationTypeInfo::Float:
    case VerificationTypeInfo::Double:
    case VerificationTypeInfo::Long: return false;
    case VerificationTypeInfo::Null:
    case VerificationTypeInfo::UninitializedThis:
    case VerificationTypeInfo::Object:
    case VerificationTypeInfo::Uninitialized: return true;
  }

  GEEVM_UNREACHBLE("Unknown VerificationTypeInfo");
}

bool isCategoryTwo(VerificationTypeInfo vti)
{
  return vti == VerificationTypeInfo::Long || vti == VerificationTypeInfo::Double;
}

class AbstractInterpreter
{
public:
  AbstractInterpreter(types::u4 pos, JMethod* method, std::vector<VerificationTypeInfo> locals, std::vector<VerificationTypeInfo>);

  void execute(types::u4 startPos);

  void push(VerificationTypeInfo vti);
  void pop(VerificationTypeInfo vti);

  void store(types::u2 idx, VerificationTypeInfo vti);
  VerificationTypeInfo load(types::u2 idx);

  void ldc(types::u2 index);
  void arrayLoad(VerificationTypeInfo vti);
  void arrayStore(VerificationTypeInfo vti);

  void binaryOp(VerificationTypeInfo vti);
  void shift(VerificationTypeInfo vti);

  void invoke(Opcode opcode);

  types::u4 nextOpcodePos(Opcode opcode);

  const std::vector<bool>& locals() const
  {
    return mLocalVariables;
  }

  const std::vector<bool>& operandStack() const
  {
    return mOperandStack;
  }

private:
  JMethod* mMethod;
  ByteStream mCode;
  std::vector<bool> mLocalVariables;
  std::vector<bool> mOperandStack;
  types::u2 mStackPointer = 0;
  types::u4 mEndPos = 0;
};

} // namespace

FrameRoots FrameRoots::compute(JMethod* method, types::u4 pos)
{
  assert(!method->isNative() && !method->isAbstract());

  // The position given by the garbage collector is the position after reading the current opcode and the bytes
  // relevant to the current opcode.
  ByteStream code{method->getCode().bytes()};

  size_t opcodePos = 0;
  while (code.pos() < pos) {
    opcodePos = code.pos();

    Opcode opcode = static_cast<Opcode>(code.readU1());
    auto skip = bytesConsumedByOpcode(opcode);

    code.skip(skip);
  }

  StackMap stackMap = StackMap::parseStackMap(method);
  const StackMap::FrameInfo& frameInfo = stackMap.frameAt(opcodePos);

  AbstractInterpreter interp{static_cast<types::u4>(opcodePos), method, frameInfo.localVariables, frameInfo.operandStack};
  interp.execute(frameInfo.startPos);

  return FrameRoots(interp.locals(), interp.operandStack());
}

std::generator<std::pair<uint16_t, Instance*>> FrameRoots::referencesInLocals(CallFrame& frame) const
{
  for (uint16_t i = 0; i < frame.currentMethod()->getCode().maxLocals(); i++) {
    if (mLocalVariableReferences[i]) {
      co_yield std::make_pair(i, std::bit_cast<Instance*>(frame.loadGenericValue(i).first));
    }
  }
}

std::generator<std::pair<uint16_t, Instance*>> FrameRoots::referencesInOperandStack(CallFrame& frame) const
{
  for (uint16_t i = 0; i < frame.stackPointer(); i++) {
    if (mOperandStackReferences[i]) {
      co_yield std::make_pair(i, std::bit_cast<Instance*>(*frame.stackElementAt(i)));
    }
  }
}

AbstractInterpreter::AbstractInterpreter(types::u4 pos, JMethod* method, std::vector<VerificationTypeInfo> locals,
                                         std::vector<VerificationTypeInfo> operandStack)
  : mMethod(method),
    mCode(method->getCode().bytes()),
    mLocalVariables(method->getCode().maxLocals(), false),
    mOperandStack(method->getCode().maxStack(), false),
    mEndPos(pos)
{
  size_t currentLocal = 0;
  size_t currentStack = 0;

  for (auto& local : locals) {
    mLocalVariables[currentLocal] = isReference(local);
    currentLocal++;

    if (isCategoryTwo(local)) {
      mLocalVariables[currentLocal] = false;
      currentLocal++;
    }
  }

  for (auto& operand : operandStack) {
    mOperandStack[currentStack] = isReference(operand);
    currentStack++;

    if (isCategoryTwo(operand)) {
      mOperandStack[currentStack] = false;
      currentStack++;
    }
  }

  mStackPointer = currentStack;
}

void AbstractInterpreter::push(VerificationTypeInfo vti)
{
  mOperandStack[mStackPointer] = isReference(vti);
  mStackPointer++;

  if (isCategoryTwo(vti)) {
    mOperandStack[mStackPointer] = false;
    mStackPointer++;
  }
}

void AbstractInterpreter::pop(VerificationTypeInfo vti)
{
  if (isCategoryTwo(vti)) {
    assert(mStackPointer > 0);
    mOperandStack[mStackPointer - 1] = false;
    mStackPointer--;
  }

  assert(mStackPointer > 0);
  mOperandStack[mStackPointer - 1] = false;
  mStackPointer--;
}

void AbstractInterpreter::ldc(types::u2 index)
{
  auto& [tag, _] = mMethod->getClass()->constantPool().getEntry(index);

  switch (tag) {
    case ConstantPool::Tag::CONSTANT_Integer: push(VerificationTypeInfo::Integer); break;
    case ConstantPool::Tag::CONSTANT_Float: push(VerificationTypeInfo::Float); break;
    case ConstantPool::Tag::CONSTANT_Double: push(VerificationTypeInfo::Double); break;
    case ConstantPool::Tag::CONSTANT_Long: push(VerificationTypeInfo::Long); break;
    case ConstantPool::Tag::CONSTANT_String:
    case ConstantPool::Tag::CONSTANT_Class: push(VerificationTypeInfo::Object); break;
    default: GEEVM_UNREACHBLE("Unknown LDC/LDC_W type!");
  }
}

void AbstractInterpreter::arrayLoad(VerificationTypeInfo vti)
{
  pop(VerificationTypeInfo::Integer);
  pop(VerificationTypeInfo::Object);

  push(vti);
}

void AbstractInterpreter::arrayStore(VerificationTypeInfo vti)
{
  pop(vti);
  pop(VerificationTypeInfo::Integer);
  pop(VerificationTypeInfo::Object);
}

void AbstractInterpreter::store(types::u2 idx, VerificationTypeInfo vti)
{
  pop(vti);
  mLocalVariables[idx] = isReference(vti);

  if (isCategoryTwo(vti)) {
    mLocalVariables[idx + 1] = false;
  }
}

void AbstractInterpreter::binaryOp(VerificationTypeInfo vti)
{
  pop(vti);
  pop(vti);
  push(vti);
}

void AbstractInterpreter::shift(VerificationTypeInfo vti)
{
  pop(VerificationTypeInfo::Integer);
  pop(vti);
  push(vti);
}

void AbstractInterpreter::invoke(Opcode opcode)
{
  size_t opcodePos = mCode.pos() - 1;
  const JMethod* method = nullptr;
  RuntimeConstantPool& rt = mMethod->getClass()->runtimeConstantPool();

  switch (opcode) {
    case Opcode::INVOKEVIRTUAL:
    case Opcode::INVOKESPECIAL:
    case Opcode::INVOKESTATIC: method = rt.getMethodRef(mCode.readU2()); break;
    case Opcode::INVOKEINTERFACE:
      method = rt.getMethodRef(mCode.readU2());
      // Consume 'count' and '0'
      mCode.skip(2);
      break;
    default: GEEVM_UNREACHBLE("Unknown invoke opcode!");
  }

  uint16_t numArgs = method->descriptor().numParameterSlots();
  if (!method->isStatic()) {
    numArgs++;
  }
  mStackPointer -= numArgs;

  if (!method->descriptor().returnType().isVoid()) {
    push(fieldTypeToVerificationTypeInfo(method->descriptor().returnType().getType()));
  }
}

void AbstractInterpreter::execute(types::u4 startPos)
{
  mCode.skip(startPos);

  while (mCode.pos() < mEndPos) {
    auto opcode = static_cast<Opcode>(mCode.readU1());

    switch (opcode) {
      case Opcode::NOP: break;
      case Opcode::ACONST_NULL: push(VerificationTypeInfo::Null); break;
      case Opcode::ICONST_M1:
      case Opcode::ICONST_0:
      case Opcode::ICONST_1:
      case Opcode::ICONST_2:
      case Opcode::ICONST_3:
      case Opcode::ICONST_4:
      case Opcode::ICONST_5: push(VerificationTypeInfo::Integer); break;
      case Opcode::LCONST_0:
      case Opcode::LCONST_1: push(VerificationTypeInfo::Long); break;
      case Opcode::FCONST_0:
      case Opcode::FCONST_1:
      case Opcode::FCONST_2: push(VerificationTypeInfo::Float); break;
      case Opcode::DCONST_0:
      case Opcode::DCONST_1: push(VerificationTypeInfo::Double); break;
      case Opcode::BIPUSH:
        mCode.readU1();
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::SIPUSH:
        mCode.readU2();
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::LDC: ldc(mCode.readU1()); break;
      case Opcode::LDC_W:
      case Opcode::LDC2_W: ldc(mCode.readU2()); break;
      case Opcode::ILOAD:
        mCode.readU1();
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::LLOAD:
        mCode.readU1();
        push(VerificationTypeInfo::Long);
        break;
      case Opcode::FLOAD:
        mCode.readU1();
        push(VerificationTypeInfo::Float);
        break;
      case Opcode::DLOAD:
        mCode.readU1();
        push(VerificationTypeInfo::Double);
        break;
      case Opcode::ALOAD:
        mCode.readU1();
        push(VerificationTypeInfo::Object);
        break;
      case Opcode::ILOAD_0:
      case Opcode::ILOAD_1:
      case Opcode::ILOAD_2:
      case Opcode::ILOAD_3: push(VerificationTypeInfo::Integer); break;
      case Opcode::LLOAD_0:
      case Opcode::LLOAD_1:
      case Opcode::LLOAD_2:
      case Opcode::LLOAD_3: push(VerificationTypeInfo::Long); break;
      case Opcode::FLOAD_0:
      case Opcode::FLOAD_1:
      case Opcode::FLOAD_2:
      case Opcode::FLOAD_3: push(VerificationTypeInfo::Float); break;
      case Opcode::DLOAD_0:
      case Opcode::DLOAD_1:
      case Opcode::DLOAD_2:
      case Opcode::DLOAD_3: push(VerificationTypeInfo::Double); break;
      case Opcode::ALOAD_0:
      case Opcode::ALOAD_1:
      case Opcode::ALOAD_2:
      case Opcode::ALOAD_3: push(VerificationTypeInfo::Object); break;
      case Opcode::IALOAD: arrayLoad(VerificationTypeInfo::Integer); break;
      case Opcode::LALOAD: arrayLoad(VerificationTypeInfo::Long); break;
      case Opcode::FALOAD: arrayLoad(VerificationTypeInfo::Float); break;
      case Opcode::DALOAD: arrayLoad(VerificationTypeInfo::Double); break;
      case Opcode::AALOAD: arrayLoad(VerificationTypeInfo::Object); break;
      case Opcode::BALOAD:
      case Opcode::CALOAD:
      case Opcode::SALOAD: arrayLoad(VerificationTypeInfo::Integer); break;
      case Opcode::ISTORE: store(mCode.readU1(), VerificationTypeInfo::Integer); break;
      case Opcode::LSTORE: store(mCode.readU1(), VerificationTypeInfo::Long); break;
      case Opcode::FSTORE: store(mCode.readU1(), VerificationTypeInfo::Float); break;
      case Opcode::DSTORE: store(mCode.readU1(), VerificationTypeInfo::Double); break;
      case Opcode::ASTORE: store(mCode.readU1(), VerificationTypeInfo::Object); break;
      case Opcode::ISTORE_0: store(0, VerificationTypeInfo::Integer); break;
      case Opcode::ISTORE_1: store(1, VerificationTypeInfo::Integer); break;
      case Opcode::ISTORE_2: store(2, VerificationTypeInfo::Integer); break;
      case Opcode::ISTORE_3: store(3, VerificationTypeInfo::Integer); break;
      case Opcode::LSTORE_0: store(0, VerificationTypeInfo::Long); break;
      case Opcode::LSTORE_1: store(1, VerificationTypeInfo::Long); break;
      case Opcode::LSTORE_2: store(2, VerificationTypeInfo::Long); break;
      case Opcode::LSTORE_3: store(3, VerificationTypeInfo::Long); break;
      case Opcode::FSTORE_0: store(0, VerificationTypeInfo::Float); break;
      case Opcode::FSTORE_1: store(1, VerificationTypeInfo::Float); break;
      case Opcode::FSTORE_2: store(2, VerificationTypeInfo::Float); break;
      case Opcode::FSTORE_3: store(3, VerificationTypeInfo::Float); break;
      case Opcode::DSTORE_0: store(0, VerificationTypeInfo::Double); break;
      case Opcode::DSTORE_1: store(1, VerificationTypeInfo::Double); break;
      case Opcode::DSTORE_2: store(2, VerificationTypeInfo::Double); break;
      case Opcode::DSTORE_3: store(3, VerificationTypeInfo::Double); break;
      case Opcode::ASTORE_0: store(0, VerificationTypeInfo::Object); break;
      case Opcode::ASTORE_1: store(1, VerificationTypeInfo::Object); break;
      case Opcode::ASTORE_2: store(2, VerificationTypeInfo::Object); break;
      case Opcode::ASTORE_3: store(3, VerificationTypeInfo::Object); break;
      case Opcode::IASTORE: arrayStore(VerificationTypeInfo::Integer); break;
      case Opcode::LASTORE: arrayStore(VerificationTypeInfo::Long); break;
      case Opcode::FASTORE: arrayStore(VerificationTypeInfo::Float); break;
      case Opcode::DASTORE: arrayStore(VerificationTypeInfo::Double); break;
      case Opcode::AASTORE: arrayStore(VerificationTypeInfo::Object); break;
      case Opcode::BASTORE:
      case Opcode::CASTORE:
      case Opcode::SASTORE: arrayStore(VerificationTypeInfo::Integer); break;
      case Opcode::POP: mStackPointer--; break;
      case Opcode::POP2:
        mStackPointer--;
        mStackPointer--;
        break;
      case Opcode::DUP: {
        bool isReference = mOperandStack[mStackPointer - 1];
        mStackPointer--;

        mOperandStack[mStackPointer] = isReference;
        mStackPointer++;
        mOperandStack[mStackPointer] = isReference;
        mStackPointer++;

        break;
      }
      case Opcode::DUP_X1: {
        bool value1 = mOperandStack[mStackPointer - 1];
        mStackPointer--;

        bool value2 = mOperandStack[mStackPointer - 1];
        mStackPointer--;

        mOperandStack[mStackPointer++] = value1;
        mOperandStack[mStackPointer++] = value2;
        mOperandStack[mStackPointer++] = value1;

        break;
      }
      case Opcode::DUP_X2: {
        bool value1 = mOperandStack[mStackPointer - 1];
        mStackPointer--;
        bool value2 = mOperandStack[mStackPointer - 1];
        mStackPointer--;
        bool value3 = mOperandStack[mStackPointer - 1];
        mStackPointer--;

        mOperandStack[mStackPointer++] = value1;
        mOperandStack[mStackPointer++] = value3;
        mOperandStack[mStackPointer++] = value2;
        mOperandStack[mStackPointer++] = value1;

        break;
      }
      case Opcode::DUP2: {
        bool value1 = mOperandStack[mStackPointer - 1];
        mStackPointer--;

        bool value2 = mOperandStack[mStackPointer - 1];
        mStackPointer--;

        mOperandStack[mStackPointer++] = value2;
        mOperandStack[mStackPointer++] = value1;
        mOperandStack[mStackPointer++] = value2;
        mOperandStack[mStackPointer++] = value1;
        break;
      }
      case Opcode::DUP2_X1: {
        bool value1 = mOperandStack[mStackPointer - 1];
        mStackPointer--;
        bool value2 = mOperandStack[mStackPointer - 1];
        mStackPointer--;
        bool value3 = mOperandStack[mStackPointer - 1];
        mStackPointer--;

        mOperandStack[mStackPointer++] = value2;
        mOperandStack[mStackPointer++] = value1;
        mOperandStack[mStackPointer++] = value3;
        mOperandStack[mStackPointer++] = value2;
        mOperandStack[mStackPointer++] = value1;

        break;
      }
      case Opcode::DUP2_X2: {
        bool value1 = mOperandStack[mStackPointer - 1];
        mStackPointer--;
        bool value2 = mOperandStack[mStackPointer - 1];
        mStackPointer--;
        bool value3 = mOperandStack[mStackPointer - 1];
        mStackPointer--;
        bool value4 = mOperandStack[mStackPointer - 1];
        mStackPointer--;

        mOperandStack[mStackPointer++] = value2;
        mOperandStack[mStackPointer++] = value1;
        mOperandStack[mStackPointer++] = value4;
        mOperandStack[mStackPointer++] = value3;
        mOperandStack[mStackPointer++] = value2;
        mOperandStack[mStackPointer++] = value1;

        break;
      }
      case Opcode::SWAP: {
        bool value1 = mOperandStack[mStackPointer - 1];
        mStackPointer--;
        bool value2 = mOperandStack[mStackPointer - 1];
        mStackPointer--;

        mOperandStack[mStackPointer++] = value1;
        mOperandStack[mStackPointer++] = value2;
        break;
      }
      case Opcode::IADD: binaryOp(VerificationTypeInfo::Integer); break;
      case Opcode::LADD: binaryOp(VerificationTypeInfo::Long); break;
      case Opcode::FADD: binaryOp(VerificationTypeInfo::Float); break;
      case Opcode::DADD: binaryOp(VerificationTypeInfo::Double); break;
      case Opcode::ISUB: binaryOp(VerificationTypeInfo::Integer); break;
      case Opcode::LSUB: binaryOp(VerificationTypeInfo::Long); break;
      case Opcode::FSUB: binaryOp(VerificationTypeInfo::Float); break;
      case Opcode::DSUB: binaryOp(VerificationTypeInfo::Double); break;
      case Opcode::IMUL: binaryOp(VerificationTypeInfo::Integer); break;
      case Opcode::LMUL: binaryOp(VerificationTypeInfo::Long); break;
      case Opcode::FMUL: binaryOp(VerificationTypeInfo::Float); break;
      case Opcode::DMUL: binaryOp(VerificationTypeInfo::Double); break;
      case Opcode::IDIV: binaryOp(VerificationTypeInfo::Integer); break;
      case Opcode::LDIV: binaryOp(VerificationTypeInfo::Long); break;
      case Opcode::FDIV: binaryOp(VerificationTypeInfo::Float); break;
      case Opcode::DDIV: binaryOp(VerificationTypeInfo::Double); break;
      case Opcode::IREM: binaryOp(VerificationTypeInfo::Integer); break;
      case Opcode::LREM: binaryOp(VerificationTypeInfo::Long); break;
      case Opcode::FREM: binaryOp(VerificationTypeInfo::Float); break;
      case Opcode::DREM: binaryOp(VerificationTypeInfo::Double); break;
      case Opcode::INEG:
      case Opcode::LNEG:
      case Opcode::FNEG:
      case Opcode::DNEG:
        // These operations pop and then push and operand of the same type, so they can be considered no-op in terms of types
        break;
      case Opcode::ISHL: shift(VerificationTypeInfo::Integer); break;
      case Opcode::LSHL: shift(VerificationTypeInfo::Long); break;
      case Opcode::ISHR: shift(VerificationTypeInfo::Integer); break;
      case Opcode::LSHR: shift(VerificationTypeInfo::Long); break;
      case Opcode::IUSHR: shift(VerificationTypeInfo::Integer); break;
      case Opcode::LUSHR: shift(VerificationTypeInfo::Long); break;
      case Opcode::IAND: binaryOp(VerificationTypeInfo::Integer); break;
      case Opcode::LAND: binaryOp(VerificationTypeInfo::Long); break;
      case Opcode::IOR: binaryOp(VerificationTypeInfo::Integer); break;
      case Opcode::LOR: binaryOp(VerificationTypeInfo::Long); break;
      case Opcode::IXOR: binaryOp(VerificationTypeInfo::Integer); break;
      case Opcode::LXOR: binaryOp(VerificationTypeInfo::Long); break;
      case Opcode::IINC: {
        auto index = mCode.readU1();
        mCode.readU1();

        mLocalVariables[index] = false;
        break;
      }
      case Opcode::I2L:
        pop(VerificationTypeInfo::Integer);
        push(VerificationTypeInfo::Long);
        break;
      case Opcode::I2F:
        pop(VerificationTypeInfo::Integer);
        push(VerificationTypeInfo::Float);
        break;
      case Opcode::I2D:
        pop(VerificationTypeInfo::Integer);
        push(VerificationTypeInfo::Double);
        break;
      case Opcode::L2I:
        pop(VerificationTypeInfo::Long);
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::L2F:
        pop(VerificationTypeInfo::Long);
        push(VerificationTypeInfo::Float);
        break;
      case Opcode::L2D:
        pop(VerificationTypeInfo::Long);
        push(VerificationTypeInfo::Double);
        break;
      case Opcode::F2I:
        pop(VerificationTypeInfo::Float);
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::F2L:
        pop(VerificationTypeInfo::Float);
        push(VerificationTypeInfo::Long);
        break;
      case Opcode::F2D:
        pop(VerificationTypeInfo::Float);
        push(VerificationTypeInfo::Double);
        break;
      case Opcode::D2I:
        pop(VerificationTypeInfo::Double);
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::D2L:
        pop(VerificationTypeInfo::Double);
        push(VerificationTypeInfo::Long);
        break;
      case Opcode::D2F:
        pop(VerificationTypeInfo::Double);
        push(VerificationTypeInfo::Float);
        break;
      case Opcode::I2B:
      case Opcode::I2C:
      case Opcode::I2S:
        pop(VerificationTypeInfo::Integer);
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::LCMP:
        pop(VerificationTypeInfo::Long);
        pop(VerificationTypeInfo::Long);
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::FCMPL:
      case Opcode::FCMPG:
        pop(VerificationTypeInfo::Float);
        pop(VerificationTypeInfo::Float);
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::DCMPL:
      case Opcode::DCMPG:
        pop(VerificationTypeInfo::Double);
        pop(VerificationTypeInfo::Double);
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::IFEQ:
      case Opcode::IFNE:
      case Opcode::IFLT:
      case Opcode::IFGE:
      case Opcode::IFGT:
      case Opcode::IFLE:
        mCode.readU2();
        pop(VerificationTypeInfo::Integer);
        break;
      case Opcode::IF_ICMPEQ:
      case Opcode::IF_ICMPNE:
      case Opcode::IF_ICMPLT:
      case Opcode::IF_ICMPGE:
      case Opcode::IF_ICMPGT:
      case Opcode::IF_ICMPLE:
        mCode.readU2();
        pop(VerificationTypeInfo::Integer);
        pop(VerificationTypeInfo::Integer);
        break;
      case Opcode::IF_ACMPEQ:
      case Opcode::IF_ACMPNE:
        mCode.readU2();
        pop(VerificationTypeInfo::Object);
        pop(VerificationTypeInfo::Object);
        break;
      case Opcode::GOTO:
      case Opcode::JSR:
      case Opcode::RET:
      case Opcode::TABLESWITCH:
      case Opcode::LOOKUPSWITCH:
      case Opcode::IRETURN:
      case Opcode::LRETURN:
      case Opcode::FRETURN:
      case Opcode::DRETURN:
      case Opcode::ARETURN:
      case Opcode::RETURN: GEEVM_UNREACHBLE("Stack map calculation encountered a control-flow instruction");
      case Opcode::GETSTATIC: {
        auto index = mCode.readU2();
        const JField* field = mMethod->getClass()->runtimeConstantPool().getFieldRef(index);
        push(fieldTypeToVerificationTypeInfo(field->fieldType()));
        break;
      }
      case Opcode::PUTSTATIC: {
        auto index = mCode.readU2();
        const JField* field = mMethod->getClass()->runtimeConstantPool().getFieldRef(index);
        pop(fieldTypeToVerificationTypeInfo(field->fieldType()));
        break;
      }
      case Opcode::GETFIELD: {
        auto index = mCode.readU2();
        const JField* field = mMethod->getClass()->runtimeConstantPool().getFieldRef(index);

        pop(VerificationTypeInfo::Object);
        push(fieldTypeToVerificationTypeInfo(field->fieldType()));
        break;
      }
      case Opcode::PUTFIELD: {
        auto index = mCode.readU2();
        const JField* field = mMethod->getClass()->runtimeConstantPool().getFieldRef(index);

        pop(fieldTypeToVerificationTypeInfo(field->fieldType()));
        pop(VerificationTypeInfo::Object);

        break;
      }
      case Opcode::INVOKEVIRTUAL:
      case Opcode::INVOKESPECIAL:
      case Opcode::INVOKESTATIC:
      case Opcode::INVOKEINTERFACE: invoke(opcode); break;
      case Opcode::INVOKEDYNAMIC: geevm_panic("Unsupported opcode"); break;
      case Opcode::NEW: {
        types::u4 opcodePos = mCode.pos() - 1;

        // Consume index
        mCode.readU2();
        push(VerificationTypeInfo::Uninitialized);
        break;
      }
      case Opcode::NEWARRAY: {
        types::u4 opcodePos = mCode.pos() - 1;
        mCode.readU1();
        pop(VerificationTypeInfo::Integer);

        if (opcodePos != mEndPos) {
          push(VerificationTypeInfo::Object);
        }
        break;
      }
      case Opcode::ANEWARRAY: {
        types::u4 opcodePos = mCode.pos() - 1;
        mCode.readU2();
        pop(VerificationTypeInfo::Integer);

        if (opcodePos != mEndPos) {
          push(VerificationTypeInfo::Object);
        }
        break;
      }
      case Opcode::ARRAYLENGTH:
        pop(VerificationTypeInfo::Object);
        push(VerificationTypeInfo::Integer);
        break;
      case Opcode::ATHROW: geevm_panic("Unexpected control-flow opcode"); break;
      case Opcode::CHECKCAST: {
        types::u2 index = mCode.readU2();
        pop(VerificationTypeInfo::Object);
        push(VerificationTypeInfo::Object);
        break;
      }
      case Opcode::INSTANCEOF: {
        types::u2 index = mCode.readU2();
        pop(VerificationTypeInfo::Object);
        push(VerificationTypeInfo::Integer);
        break;
      }
      case Opcode::MONITORENTER:
      case Opcode::MONITOREXIT: pop(VerificationTypeInfo::Object); break;
      case Opcode::WIDE: {
        Opcode modifiedOpcode = static_cast<Opcode>(mCode.readU1());
        types::u2 index = mCode.readU2();

        switch (modifiedOpcode) {
          case Opcode::IINC:
            mCode.readU2();
            push(VerificationTypeInfo::Integer);
            break;
          case Opcode::ILOAD: push(VerificationTypeInfo::Integer); break;
          case Opcode::LLOAD: push(VerificationTypeInfo::Long); break;
          case Opcode::FLOAD: push(VerificationTypeInfo::Float); break;
          case Opcode::DLOAD: push(VerificationTypeInfo::Double); break;
          case Opcode::ALOAD: push(VerificationTypeInfo::Object); break;
          case Opcode::ISTORE: store(index, VerificationTypeInfo::Integer); break;
          case Opcode::LSTORE: store(index, VerificationTypeInfo::Long); break;
          case Opcode::FSTORE: store(index, VerificationTypeInfo::Float); break;
          case Opcode::DSTORE: store(index, VerificationTypeInfo::Double); break;
          case Opcode::ASTORE: store(index, VerificationTypeInfo::Object); break;
          default: GEEVM_UNREACHBLE("Unknown opcode for WIDE");
        }

        break;
      }
      case Opcode::MULTIANEWARRAY: {
        mCode.readU2();
        mCode.readU1();

        push(VerificationTypeInfo::Object);
        break;
      }
      case Opcode::IFNULL:
      case Opcode::IFNONNULL:
        mCode.readU2();
        pop(VerificationTypeInfo::Object);
        break;
      case Opcode::GOTO_W:
      case Opcode::JSR_W: GEEVM_UNREACHBLE("Unexpected control-flow opcode"); break;
      case Opcode::BREAKPOINT:
      case Opcode::IMPDEP1:
      case Opcode::IMPDEP2:
        // No-op
        break;
    }
  }
}

namespace
{

size_t bytesConsumedByOpcode(Opcode opcode)
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
} // namespace
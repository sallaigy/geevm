#include "vm/StackMap.h"
#include "class_file/Opcode.h"
#include "common/ByteStream.h"
#include "common/JvmError.h"
#include "vm/Class.h"
#include "vm/Method.h"

using namespace geevm;

static StackMap::FrameInfo parseMethodDescriptor(const JMethod* method);
static StackMap::FrameInfo parseFrame(ByteStream& bytes, StackMap::FrameInfo& previous, int64_t* currentOffset);
static VerificationTypeInfo readTypeInfo(ByteStream& bytes);

StackMap StackMap::parseStackMap(const JMethod* method)
{
  std::vector<FrameInfo> frames;

  // Create first frame from the method descriptor
  frames.push_back(parseMethodDescriptor(method));

  // Parse the StackMapTable attribute
  auto* stackMapTableBytes = method->getCode().getAttribute(u"StackMapTable");
  if (stackMapTableBytes == nullptr) {
    return StackMap{frames};
  }

  ByteStream bytes{*stackMapTableBytes};
  types::u2 numEntries = bytes.readU2();

  int64_t currentOffset = -1;
  for (types::u2 entryIdx = 0; entryIdx < numEntries; ++entryIdx) {
    frames.push_back(parseFrame(bytes, frames.back(), &currentOffset));
  }

  return StackMap{frames};
}

const StackMap::FrameInfo& StackMap::frameAt(types::u4 pos) const
{
  for (size_t i = 0; i < mFrames.size(); ++i) {
    if (i == mFrames.size() - 1) {
      assert(mFrames[i].startPos <= pos);
      return mFrames[i];
    }

    if (mFrames[i].startPos <= pos && pos < mFrames[i + 1].startPos) {
      return mFrames[i];
    }
  }

  GEEVM_UNREACHBLE("There should be a valid stack map frame for any bytecode position");
}

static constexpr types::u1 SameFrameStart = 0;
static constexpr types::u1 SameFrameEnd = 63;

static constexpr types::u1 SameLocalsOneStackStart = 64;
static constexpr types::u1 SameLocalsOneStackEnd = 127;

static constexpr types::u1 SameLocalsOneStackItemExtended = 247;

static constexpr types::u1 ChopFrameStart = 248;
static constexpr types::u1 ChopFrameEnd = 250;

static constexpr types::u1 SameFrameExtended = 251;

static constexpr types::u1 AppendFrameStart = 252;
static constexpr types::u1 AppendFrameEnd = 254;

static constexpr types::u1 FullFrame = 255;

StackMap::FrameInfo parseFrame(ByteStream& bytes, StackMap::FrameInfo& previous, int64_t* currentOffset)
{
  auto kind = bytes.readU1();

  if (kind >= SameFrameStart && kind <= SameFrameEnd) {
    *currentOffset = *currentOffset + kind + 1;
    return StackMap::FrameInfo{static_cast<types::u4>(*currentOffset), previous.localVariables, {}};
  }

  if (kind >= SameLocalsOneStackStart && kind <= SameLocalsOneStackEnd) {
    *currentOffset = *currentOffset + (kind - 64) + 1;
    auto stackType = readTypeInfo(bytes);

    return StackMap::FrameInfo{static_cast<types::u4>(*currentOffset), previous.localVariables, std::vector{stackType}};
  }

  if (kind == SameLocalsOneStackItemExtended) {
    types::u2 offsetDelta = bytes.readU2();
    auto stackType = readTypeInfo(bytes);
    *currentOffset = *currentOffset + offsetDelta + 1;

    return StackMap::FrameInfo{static_cast<types::u4>(*currentOffset), previous.localVariables, std::vector{stackType}};
  }

  if (kind >= ChopFrameStart && kind <= ChopFrameEnd) {
    types::u2 offsetDelta = bytes.readU2();
    *currentOffset = *currentOffset + offsetDelta + 1;
    std::vector<VerificationTypeInfo> localVariables = previous.localVariables;

    localVariables.resize(localVariables.size() - (251 - kind));

    return StackMap::FrameInfo{static_cast<types::u4>(*currentOffset), localVariables, {}};
  }

  if (kind == SameFrameExtended) {
    types::u2 offsetDelta = bytes.readU2();
    *currentOffset = *currentOffset + offsetDelta + 1;

    return StackMap::FrameInfo{static_cast<types::u4>(*currentOffset), previous.localVariables, {}};
  }

  if (kind >= AppendFrameStart && kind <= AppendFrameEnd) {
    types::u2 offsetDelta = bytes.readU2();
    *currentOffset = *currentOffset + offsetDelta + 1;
    std::vector<VerificationTypeInfo> localVariables = previous.localVariables;

    for (types::u1 i = 0; i < kind - 251; i++) {
      localVariables.push_back(readTypeInfo(bytes));
    }

    return StackMap::FrameInfo{static_cast<types::u4>(*currentOffset), localVariables, {}};
  }

  if (kind == FullFrame) {
    std::vector<VerificationTypeInfo> localVariables;
    std::vector<VerificationTypeInfo> operandStack;

    types::u2 offsetDelta = bytes.readU2();
    *currentOffset = *currentOffset + offsetDelta + 1;
    types::u2 numLocals = bytes.readU2();
    for (types::u2 i = 0; i < numLocals; i++) {
      localVariables.push_back(readTypeInfo(bytes));
    }
    types::u2 numStack = bytes.readU2();
    for (types::u2 i = 0; i < numStack; i++) {
      operandStack.push_back(readTypeInfo(bytes));
    }

    return StackMap::FrameInfo{static_cast<types::u4>(*currentOffset), localVariables, operandStack};
  }

  GEEVM_UNREACHBLE("Unknown StackMapFrame kind")
}

StackMap::FrameInfo parseMethodDescriptor(const JMethod* method)
{
  std::vector<VerificationTypeInfo> types;

  if (!method->isStatic()) {
    if (method->name() == u"<init>") {
      types.push_back(VerificationTypeInfo::UninitializedThis);
    } else {
      types.push_back(VerificationTypeInfo::Object);
    }
  }

  for (const FieldType& field : method->descriptor().parameters()) {
    types.push_back(fieldTypeToVerificationTypeInfo(field));
  }

  return StackMap::FrameInfo{0, types, std::vector<VerificationTypeInfo>{}};
}

VerificationTypeInfo geevm::fieldTypeToVerificationTypeInfo(const FieldType& type)
{

  if (auto primitive = type.asPrimitive(); primitive) {
    switch (*primitive) {
      case PrimitiveType::Double: return VerificationTypeInfo::Double;
      case PrimitiveType::Float: return VerificationTypeInfo::Float;
      case PrimitiveType::Long: return VerificationTypeInfo::Long;
      // Everything else is stored as an int
      default: return VerificationTypeInfo::Integer;
    }
  }

  return VerificationTypeInfo::Object;
}

VerificationTypeInfo readTypeInfo(ByteStream& bytes)
{
  auto kind = static_cast<VerificationTypeInfo>(bytes.readU1());
  if (kind == VerificationTypeInfo::Object || kind == VerificationTypeInfo::Uninitialized) {
    // Consume the offset of these types
    bytes.skip(2);
  }

  return kind;
}

#if 0

StackMap::StackMap(InstanceClass* klass, JMethod* method)
  : mClass(klass), mMethod(method)
{
  if (mMethod->isNative() || mMethod->isAbstract()) {
    return;
  }

  // Use the descriptor to create the first frame info
  std::vector<bool> initialLocalRefs(method->getCode().maxLocals());

  types::u2 numLocals = 0;

  size_t i = 0;
  if (!method->isStatic()) {
    initialLocalRefs[i] = true;
    i += 1;
    numLocals += 1;
  }

  for (const FieldType& field : method->descriptor().parameters()) {
    initialLocalRefs[i] = field.isReferenceOrArray();
    i += 1;
    numLocals += 1;

    if (field.isCategoryTwo()) {
      initialLocalRefs[i] = false;
      i += 1;
      numLocals += 1;
    }
  }

  // Local variables derived from the descriptor, operand stack is empty
  mFrameReferences.emplace_back(0, initialLocalRefs, std::vector<bool>(method->getCode().maxStack()), 0);

  // Parse the StackMapTable
  const std::vector<types::u1>* attributeBytes = mMethod->getCode().getAttribute(u"StackMapTable");
  if (attributeBytes != nullptr) {
    ByteStream attributeBytesStream(*attributeBytes);
    this->parseStackMapTable(attributeBytesStream, numLocals);
  }
}

static bool isReferenceTypeTag(TypeInfoTag tag)
{
  switch (tag) {
    case TypeInfoTag::Top: [[fallthrough]];
    case TypeInfoTag::Integer: [[fallthrough]];
    case TypeInfoTag::Float: [[fallthrough]];
    case TypeInfoTag::Double: [[fallthrough]];
    case TypeInfoTag::Long: return false;
    case TypeInfoTag::Null: [[fallthrough]];
    case TypeInfoTag::UninitializedThis: [[fallthrough]];
    case TypeInfoTag::Object: [[fallthrough]];
    case TypeInfoTag::Uninitialized: return true;
  }

  GEEVM_UNREACHBLE("Invalid TypeInfoTag");
}

static types::u2 typeTagIncrement(TypeInfoTag tag)
{
  if (tag == TypeInfoTag::Object || tag == TypeInfoTag::Uninitialized) {
    return 2;
  }

  return 0;
}

static bool isCategoryTwo(TypeInfoTag tag)
{
  return tag == TypeInfoTag::Double || tag == TypeInfoTag::Long;
}

void StackMap::parseStackMapTable(ByteStream& attrBytes, types::u2 startNumLocals)
{
  types::u2 numberOfEntries = attrBytes.readU2();
  types::u2 numLocals = startNumLocals;
  types::u2 numStackEntries = 0;

  for (types::u2 i = 0; i < numberOfEntries; i++) {
    FrameReferences& previous = mFrameReferences.back();

    types::u2 offsetDelta = 0;
    std::vector<bool> localRefs = previous.localVariableReferences();
    std::vector<bool> stackRefs = previous.operandStackReferences();

    types::u1 frameType = attrBytes.readU1();
    if (frameType == 0 || frameType <= 63) {
      // same_frame
      numStackEntries = 0;
      offsetDelta = frameType;
    } else if (frameType >= 64 && frameType <= 127) {
      // same_locals_1_stack_item_frame
      auto typeTag = static_cast<TypeInfoTag>(attrBytes.readU1());
      attrBytes.skip(typeTagIncrement(typeTag));

      std::ranges::fill(stackRefs, false);
      stackRefs[0] = isReferenceTypeTag(typeTag);

      numStackEntries = 1;
      offsetDelta = frameType;
    } else if (frameType == 247) {
      // same_locals_1_stack_item_frame_extended
      offsetDelta = attrBytes.readU2();
      auto typeTag = static_cast<TypeInfoTag>(attrBytes.readU1());
      attrBytes.skip(typeTagIncrement(typeTag));

      std::ranges::fill(stackRefs, false);
      numStackEntries = 1;
      stackRefs[0] = isReferenceTypeTag(typeTag);
    } else if (frameType >= 248 && frameType <= 250) {
      // chop_frame
      types::u1 k = 251 - frameType;
      offsetDelta = attrBytes.readU2();

      for (types::u2 j = 0; j < k; j++) {
        localRefs[localRefs.size() - j - 1] = false;
      }
      numLocals -= k;

      std::ranges::fill(stackRefs, false);
      numStackEntries = 0;
    } else if (frameType == 251) {
      // same_frame_extended
      numStackEntries = 0;
      offsetDelta = attrBytes.readU2();
    } else if (frameType >= 252 && frameType <= 254) {
      // append_frame
      types::u1 k = frameType - 251;
      offsetDelta = attrBytes.readU2();

      size_t localIdx = numLocals;
      for (types::u2 j = numLocals; j < numLocals + k; j++) {
        auto typeTag = static_cast<TypeInfoTag>(attrBytes.readU1());
        localRefs[localIdx] = isReferenceTypeTag(typeTag);
        attrBytes.skip(typeTagIncrement(typeTag));

        localIdx += 1;
        if (isCategoryTwo(typeTag)) {
          localRefs[localIdx] = false;
          localIdx += 1;
        }
      }
      numLocals = localIdx;

      std::ranges::fill(stackRefs, false);
      numStackEntries = 0;
    } else {
      assert(frameType == 255 && "Unknown frame type in StackMapTable attribute");
      // full_frame
      offsetDelta = attrBytes.readU2();

      size_t localIdx = 0;
      types::u2 localsToRead = attrBytes.readU2();
      for (types::u2 j = 0; j < localsToRead; j++) {
        auto typeTag = static_cast<TypeInfoTag>(attrBytes.readU1());
        localRefs[localIdx] = isReferenceTypeTag(typeTag);
        attrBytes.skip(typeTagIncrement(typeTag));
        localIdx += 1;
        if (isCategoryTwo(typeTag)) {
          localRefs[localIdx] = false;
          localIdx += 1;
        }
      }

      size_t stackIdx = 0;
      types::u2 stackEntriesToRead = attrBytes.readU2();
      for (types::u2 j = 0; j < stackEntriesToRead; j++) {
        auto typeTag = static_cast<TypeInfoTag>(attrBytes.readU1());
        stackRefs[stackIdx] = isReferenceTypeTag(typeTag);
        attrBytes.skip(typeTagIncrement(typeTag));
        stackIdx += 1;
        if (isCategoryTwo(typeTag)) {
          stackRefs[stackIdx] = false;
          stackIdx += 1;
        }
      }

      numStackEntries = stackIdx;
      numLocals = localIdx;
    }

    int64_t offset = previous.offset();
    if (mFrameReferences.size() == 1) {
      offset = -1;
    }

    mFrameReferences.emplace_back(offset + offsetDelta + 1, localRefs, stackRefs, numStackEntries);
  }
}

StackMap::FrameReferences StackMap::calculateFrameReferences(types::u4 pc)
{
  assert(!mMethod->isNative() && !mMethod->isAbstract());

  // Find the appropriate stack map frame
  FrameReferences* frameRefs = nullptr;
  size_t frameEnd = mMethod->getCode().bytes().size();

  for (size_t i = 0; i < mFrameReferences.size(); ++i) {
    if (i == mFrameReferences.size() - 1) {
      // This is the last entry in the table
      if (mFrameReferences[i].offset() < pc) {
        frameRefs = &mFrameReferences[i];
      }
    } else if (mFrameReferences[i].offset() <= pc && pc <= mFrameReferences[i + 1].offset()) {
      frameRefs = &mFrameReferences[i];
      frameEnd = mFrameReferences[i + 1].offset();
    }
  }

  assert(frameRefs != nullptr);
  assert(frameRefs->offset() <= pc && pc <= frameEnd);

  std::vector<bool> stackRefs = frameRefs->operandStackReferences();
  stackRefs.resize(mMethod->getCode().maxStack());
  size_t sp = frameRefs->stackDepth();

  // Perform a abstract interpretation of the byte code in the range to determine which stack positions hold references
  ByteStream code(mMethod->getCode().bytes());
  code.skip(frameRefs->offset());

  while (code.pos() < pc) {
    Opcode opcode = static_cast<Opcode>(code.readU1());
    switch (opcode) {
      using enum Opcode;
      case ACONST_NULL: {
        stackRefs[sp++] = true;
        break;
      }
      case ICONST_M1:
      case ICONST_0:
      case ICONST_1:
      case ICONST_2:
      case ICONST_3:
      case ICONST_4:
      case FCONST_0:
      case FCONST_1:
      case FCONST_2:
      case ICONST_5: stackRefs[sp++] = false; break;
      case LCONST_0:
      case LCONST_1:
      case DCONST_0:
      case DCONST_1:
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      case BIPUSH: {
        code.skip(1);
        stackRefs[sp++] = false;
        break;
      }
      case SIPUSH: {
        code.skip(2);
        stackRefs[sp++] = false;
        break;
      }
      case LDC: {
        auto index = code.readU1();
        auto& [tag, _] = mClass->constantPool().getEntry(index);
        stackRefs[sp++] = tag == ConstantPool::Tag::CONSTANT_Class;
        break;
      }
      case LDC_W: {
        auto index = code.readU2();
        auto& [tag, _] = mClass->constantPool().getEntry(index);
        stackRefs[sp++] = tag == ConstantPool::Tag::CONSTANT_Class;
        break;
      }
      case LDC2_W: {
        code.readU2();
        // It's either double or long, so it takes two slots
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
      }
      case ILOAD:
      case FLOAD:
        code.skip(1);
        stackRefs[sp++] = false;
        break;
      case DLOAD:
      case LLOAD:
        code.skip(1);
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      case ALOAD:
        code.skip(1);
        stackRefs[sp++] = true;
        break;
      case ILOAD_0:
      case ILOAD_1:
      case ILOAD_2:
      case ILOAD_3:
      case FLOAD_0:
      case FLOAD_1:
      case FLOAD_2:
      case FLOAD_3: stackRefs[sp++] = false; break;
      case LLOAD_0:
      case LLOAD_1:
      case LLOAD_2:
      case LLOAD_3:
      case DLOAD_0:
      case DLOAD_1:
      case DLOAD_2:
      case DLOAD_3:
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      case ALOAD_0:
      case ALOAD_1:
      case ALOAD_2:
      case ALOAD_3: stackRefs[sp++] = true; break;
      case IALOAD:
      case FALOAD:
      case BALOAD:
      case CALOAD:
      case SALOAD:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
      case LALOAD:
      case DALOAD:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      case AALOAD:
        sp--;
        sp--;
        stackRefs[sp++] = true;
        break;
      case ISTORE:
      case FSTORE:
        code.skip(1);
        sp--;
        break;
      case LSTORE:
      case DSTORE:
        code.skip(1);
        sp--;
        sp--;
        break;
      case ISTORE_0:
      case ISTORE_1:
      case ISTORE_2:
      case ISTORE_3:
      case FSTORE_0:
      case FSTORE_1:
      case FSTORE_2:
      case FSTORE_3:
      case ASTORE_0:
      case ASTORE_1:
      case ASTORE_2:
      case ASTORE_3: sp--; break;
      case LSTORE_0:
      case LSTORE_1:
      case LSTORE_2:
      case LSTORE_3:
      case DSTORE_0:
      case DSTORE_1:
      case DSTORE_2:
      case DSTORE_3:
        sp--;
        sp--;
        break;
      // Array stores
      case IASTORE:
      case LASTORE:
      case FASTORE:
      case DASTORE:
      case AASTORE:
      case BASTORE:
      case CASTORE:
      case SASTORE:
        sp--;
        sp--;
        break;
      // Stack manipulation
      case POP: sp--; break;
      case POP2:
        sp--;
        sp--;
        break;
      case IADD:
      case FADD:
      case ISUB:
      case FSUB:
      case IMUL:
      case FMUL:
      case IDIV:
      case FDIV:
      case IREM:
      case FREM:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
      case LADD:
      case DADD:
      case LSUB:
      case DSUB:
      case LMUL:
      case DMUL:
      case LDIV:
      case DDIV:
      case LREM:
      case DREM:
        sp--;
        sp--;
        sp--;
        sp--;
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      case INEG:
      case FNEG:
        sp--;
        stackRefs[sp++] = false;
        break;
      case LNEG:
      case DNEG:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      // Bit-shift operators
      case ISHL:
      case ISHR:
      case IUSHR:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
      case LSHL:
      case LSHR:
      case LUSHR:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      // Bit-logic
      case IAND:
      case IOR:
      case IXOR:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
      case LAND:
      case LOR:
      case LXOR:
        sp--;
        sp--;
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
      case IINC: code.skip(2); break;
      // Casts
      case I2L:
        sp--;
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      case I2F:
        sp--;
        stackRefs[sp++] = false;
        break;
      case I2D:
        sp--;
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      case L2I:
      case L2F:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
      case L2D:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      case F2I:
        sp--;
        stackRefs[sp++] = false;
        break;
      case F2L:
      case F2D:
        sp--;
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;
      case D2I:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
      case D2L:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        stackRefs[sp++] = false;
        break;

      case D2F:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
      case I2B:
      case I2C:
      case I2S:
        sp--;
        stackRefs[sp++] = false;
        break;
      // Comparisons
      case LCMP:
      case DCMPL:
      case DCMPG:
        sp--;
        sp--;
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
      case FCMPL:
      case FCMPG:
        sp--;
        sp--;
        stackRefs[sp++] = false;
        break;
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
      case JSR:
      case RET:
      case TABLESWITCH:
      case LOOKUPSWITCH:
      case IRETURN:
      case LRETURN:
      case FRETURN:
      case DRETURN:
      case ARETURN:
      case RETURN: {
        geevm_panic("Unsupported operation in stack analysis");
        break;
      }
      case GETSTATIC: {
        auto index = code.readU2();
        const JField* field = mClass->runtimeConstantPool().getFieldRef(index);

        stackRefs[sp++] = field->fieldType().isReferenceOrArray();
        if (field->fieldType().isCategoryTwo()) {
          sp++;
        }
        break;
      }
      case PUTSTATIC: {
        auto index = code.readU2();
        const JField* field = mClass->runtimeConstantPool().getFieldRef(index);

        sp--;
        if (field->fieldType().isCategoryTwo()) {
          sp--;
        }
        break;
      }
      case GETFIELD: {
        auto index = code.readU2();
        const JField* field = mClass->runtimeConstantPool().getFieldRef(index);

        // objectref
        sp--;
        stackRefs[sp++] = field->fieldType().isReferenceOrArray();
        if (field->fieldType().isCategoryTwo()) {
          sp++;
        }
        break;
      }
      case PUTFIELD: {
        auto index = code.readU2();
        const JField* field = mClass->runtimeConstantPool().getFieldRef(index);

        if (field->fieldType().isCategoryTwo()) {
          sp--;
        }
        sp--;
        sp--;

        break;
      }
      case INVOKEVIRTUAL:
      case INVOKESPECIAL:
      case INVOKESTATIC: {
        auto index = code.readU2();
        const JMethod* method = mClass->runtimeConstantPool().getMethodRef(index);
        for (int i = 0; i < method->descriptor().numParameterSlots(); i++) {
          sp--;
        }

        if (method->isStatic()) {
          sp--;
        }

        auto returnTy = method->descriptor().returnType();
        if (!returnTy.isVoid()) {
          stackRefs[sp++] = returnTy.getType().isReferenceOrArray();
          if (returnTy.getType().isCategoryTwo()) {
            sp++;
          }
        }
        break;
      }
      case INVOKEINTERFACE: {
        auto index = code.readU2();
        const JMethod* method = mClass->runtimeConstantPool().getMethodRef(index);

        code.readU2();

        for (int i = 0; i < method->descriptor().numParameterSlots(); i++) {
          sp--;
        }

        auto returnTy = method->descriptor().returnType();
        if (!returnTy.isVoid()) {
          stackRefs[sp++] = returnTy.getType().isReferenceOrArray();
          if (returnTy.getType().isCategoryTwo()) {
            sp++;
          }
        }
        break;
      }
      case INVOKEDYNAMIC: geevm_panic("Unsupported operation in stack analysis");
      // OOP
      case NEW:
        code.readU2();
        stackRefs[sp++] = true;
        break;
      case NEWARRAY: {
        code.readU1();
        sp--;
        stackRefs[sp++] = true;
        break;
      }
      case ANEWARRAY: {
        code.readU2();
        sp--;
        stackRefs[sp++] = true;
        break;
      }
      case ARRAYLENGTH: {
        sp--;
        stackRefs[sp++] = false;
        break;
      }
      case ATHROW: sp--; break;
      case CHECKCAST: {
        code.readU2();
        sp--;
        stackRefs[sp++] = true;
        break;
      }
      case INSTANCEOF: {
        code.readU2();
        sp--;
        stackRefs[sp++] = false;
        break;
      }
      case MONITORENTER:
      case MONITOREXIT: sp--; break;
      case WIDE:
      case MULTIANEWARRAY: {
        code.readU2();
        uint8_t dimensions = code.readU1();
        for (uint8_t dim = 0; dim < dimensions; dim++) {
          sp--;
        }
        stackRefs[sp++] = true;
        break;
      }
      case IFNULL:
      case IFNONNULL: sp--; break;
      case GOTO_W:
      case JSR_W:
      case BREAKPOINT:
      case IMPDEP1:
      case IMPDEP2: break;
      default: GEEVM_UNREACHBLE("Unknown opcode!");
    }
  }

  for (uint16_t i = sp; i < stackRefs.size(); i++) {
    stackRefs[i] = false;
  }
}

#endif

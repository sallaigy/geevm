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

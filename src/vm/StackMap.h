#ifndef GEEVM_VM_STACKMAP_H
#define GEEVM_VM_STACKMAP_H

#include "common/JvmTypes.h"
#include "vm/Method.h"

#include <vector>

namespace geevm
{

class JMethod;
class ByteStream;
class InstanceClass;

enum class VerificationTypeInfo : types::u1
{
  Top = 0,
  Integer = 1,
  Float = 2,
  Double = 3,
  Long = 4,
  Null = 5,
  UninitializedThis = 6,
  Object = 7,
  Uninitialized = 8,
};

VerificationTypeInfo fieldTypeToVerificationTypeInfo(const FieldType& type);

class StackMap
{
public:
  struct FrameInfo
  {
    types::u4 startPos;
    std::vector<VerificationTypeInfo> localVariables;
    std::vector<VerificationTypeInfo> operandStack;
  };

private:
  explicit StackMap(std::vector<FrameInfo> frames)
    : mFrames(std::move(frames))
  {
  }

public:
  /// Parses the "StackMap" attribute of the given non-native method.
  static StackMap parseStackMap(const JMethod* method);

  /// Returns the active frame information at the given index.
  const FrameInfo& frameAt(types::u4 index) const;

private:
  const std::vector<FrameInfo> mFrames;
};

} // namespace geevm

#endif // GEEVM_VM_STACKMAP_H

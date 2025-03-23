#ifndef GEEVM_COMMON_BYTESTREAM_H
#define GEEVM_COMMON_BYTESTREAM_H

#include "common/JvmTypes.h"

#include <span>

namespace geevm
{

class ByteStream
{
public:
  explicit ByteStream(std::span<const types::u1> bytes)
    : mBytes(bytes), mPos(0)
  {
  }

  types::u1 readU1()
  {
    return (mBytes)[mPos++];
  }

  types::u2 readU2()
  {
    types::u2 value = ((mBytes)[mPos] << 8u) | (mBytes)[mPos + 1];
    mPos += 2;
    return value;
  }

  types::u4 readU4()
  {
    types::u4 value = ((mBytes)[mPos] << 24u) | ((mBytes)[mPos + 1] << 16u) | ((mBytes)[mPos + 2] << 8u) | (mBytes)[mPos + 3];
    mPos += 4;
    return value;
  }

  void skip(size_t count)
  {
    mPos += count;
  }

  size_t pos() const
  {
    return mPos;
  }

  size_t size() const
  {
    return mBytes.size();
  }

private:
  std::span<const types::u1> mBytes;
  size_t mPos;
};

} // namespace geevm

#endif // GEEVM_COMMON_BYTESTREAM_H

#ifndef GEEVM_COMMON_VMERROR_H
#define GEEVM_COMMON_VMERROR_H

#include "common/JvmTypes.h"

#include <expected>
#include <memory>

namespace geevm
{

class VmError
{
public:
  VmError() = default;

  explicit VmError(types::JString message)
    : mMessage(std::move(message))
  {
  }

  VmError(const VmError&) = default;

  types::JString message() const
  {
    return mMessage;
  }

private:
  types::JString mMessage;
};

class NoClassDefFoundError : public VmError
{
public:
  NoClassDefFoundError() = default;
  using VmError::VmError;
};

template<class T>
using JvmExpected = std::expected<T, std::unique_ptr<VmError>>;

template<class T, class E, class... Args>
JvmExpected<T> makeError(Args&&... args)
{
  return std::unexpected(std::make_unique<E>(args...));
}

template<class T>
JvmExpected<T> makeError(std::unique_ptr<VmError> error)
{
  return std::unexpected(std::move(error));
}

} // namespace geevm
#endif // GEEVM_COMMON_VMERROR_H

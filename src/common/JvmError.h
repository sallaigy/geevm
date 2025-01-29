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

  explicit VmError(types::JString exception, types::JString message = u"")
    : mException(std::move(exception)), mMessage(std::move(message))
  {
  }

  VmError(const VmError&) = default;

  const types::JString& exception() const
  {
    return mException;
  }

  const types::JString& message() const
  {
    return mMessage;
  }

private:
  types::JString mException;
  types::JString mMessage;
};

template<class T>
using JvmExpected = std::expected<T, VmError>;

template<class T, class... Args>
JvmExpected<T> makeError(Args&&... args)
{
  return std::unexpected(VmError{args...});
}

[[noreturn]] void geevm_panic(std::string_view message);

} // namespace geevm
#endif // GEEVM_COMMON_VMERROR_H

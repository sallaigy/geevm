#ifndef GEEVM_COMMON_TYPETRAITS_H
#define GEEVM_COMMON_TYPETRAITS_H

#include <type_traits>

namespace geevm
{

template<class Ty, class... Ts>
constexpr bool is_one_of() noexcept
{
  return (std::is_same_v<Ty, Ts> || ...);
}

} // namespace geevm

#endif // GEEVM_COMMON_TYPETRAITS_H

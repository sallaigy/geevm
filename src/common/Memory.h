#ifndef GEEVM_COMMON_MEMORY_H
#define GEEVM_COMMON_MEMORY_H

inline std::size_t alignTo(size_t size, size_t alignment)
{
  return (size + (alignment - 1)) & ~(alignment - 1);
}

#endif // GEEVM_COMMON_MEMORY_H

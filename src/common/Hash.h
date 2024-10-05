#ifndef GEEVM_COMMON_HASH_H
#define GEEVM_COMMON_HASH_H

#include "common/JvmTypes.h"

namespace geevm
{

struct PairHash
{
  std::size_t operator()(const NameAndDescriptor& pair) const
  {
    std::size_t hash = 17;
    hash = hash * 31 + std::hash<types::JString>()(pair.first);
    return hash * 31 + std::hash<types::JString>()(pair.second);
  }
};

}

#endif //GEEVM_COMMON_HASH_H

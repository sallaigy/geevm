#include "common/Debug.h"

#include <iostream>

void geevm::debug::geevm_unreachable(const char* message, const char* file, int line)
{
  std::cerr << "Unreachable executed at " << file << " line " << line << ": " << message << std::endl;
  abort();
}

#include "common/JvmError.h"

#include <iostream>

void geevm::geevm_panic(const char* message)
{
  std::cerr << "PANIC: " << message << std::endl;
  std::exit(1);
}

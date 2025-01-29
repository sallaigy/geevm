#include "common/JvmError.h"

#include <iostream>

void geevm::geevm_panic(std::string_view message)
{
  std::cerr << "PANIC: " << message << std::endl;
  std::exit(1);
}

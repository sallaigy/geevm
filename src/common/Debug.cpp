#include "common/Debug.h"

#include <format>
#include <iostream>

void geevm::debug::geevm_unreachable(const char* message, const char* file, int line)
{
  std::cerr << "Unreachable executed at " << file << " line " << line << ": " << message << std::endl;
  abort();
}

void geevm::debug::DebugLogger::addComponent(const std::string& component)
{
  mComponents.insert(component);
}

void geevm::debug::DebugLogger::doLog(const std::string& component, const std::string& message)
{
  if (mComponents.contains(component)) {
    std::cerr << std::format("[{}] {}", component, message) << std::endl;
  }
}

geevm::debug::DebugLogger& geevm::debug::DebugLogger::get()
{
  static DebugLogger instance;
  return instance;
}

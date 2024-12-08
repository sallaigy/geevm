#ifndef GEEVM_DYNAMICLIBRARY_H
#define GEEVM_DYNAMICLIBRARY_H

#include <memory>

namespace geevm
{

class DynamicLibrary
{
protected:
  DynamicLibrary() = default;

public:
  static std::unique_ptr<DynamicLibrary> create();

  virtual ~DynamicLibrary() = default;

  virtual void* findSymbol(const char* symbol) const = 0;
};

} // namespace geevm

#endif // GEEVM_DYNAMICLIBRARY_H

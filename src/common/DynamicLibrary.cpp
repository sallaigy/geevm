#include "common/DynamicLibrary.h"

#include <dlfcn.h>

using namespace geevm;

namespace
{

class UnixDynamicLibary : public DynamicLibrary
{
public:
  void* findSymbol(const char* symbol) const override;

private:
  void* mHandle = nullptr;
};

} // namespace

std::unique_ptr<DynamicLibrary> DynamicLibrary::create()
{
  return std::make_unique<UnixDynamicLibary>();
}

void* UnixDynamicLibary::findSymbol(const char* symbol) const
{
  void* handle = dlsym(mHandle, symbol);
  const char* error = dlerror();
  if (error) {
    return nullptr;
  }

  return handle;
}

#include "vm/Thread.h"
#include "vm/Vm.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[])
{
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <main class>" << std::endl;
    return 1;
  }

  auto mainClassName = geevm::types::convertString(argv[1]);
  std::replace(mainClassName.begin(), mainClassName.end(), u'.', u'/');

  auto vm = std::make_unique<geevm::Vm>();
  assert(std::getenv("JDK17_PATH") != nullptr);

  vm->bootstrapClassLoader().classPath().addDirectory(std::getenv("JDK17_PATH"));

  auto baseClassLoader = std::make_unique<geevm::BaseClassLoader>();
  baseClassLoader->classPath().addDirectory(std::filesystem::current_path().string());

  vm->bootstrapClassLoader().registerClassLoader(std::move(baseClassLoader));

  vm->initialize();

  auto mainClass = vm->resolveClass(mainClassName);

  if (!mainClass) {
    return 1;
  }

  auto mainMethod = (*mainClass)->getMethod(u"main", u"([Ljava/lang/String;)V");
  if (!mainMethod) {
    // TODO: Raise error
    return 1;
  }

  vm->mainThread().start(*mainMethod, {});

  return 0;
}
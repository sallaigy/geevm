#include "common/DynamicLibrary.h"
#include "vm/Thread.h"
#include "vm/Vm.h"

#include <algorithm>
#include <argparse/argparse.hpp>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[])
{
  argparse::ArgumentParser program("java");
  program.add_argument("mainclass");
  // Heap behavior
  program.add_argument("-Xgc-after-every-alloc").hidden().flag();
  // Initialization
  program.add_argument("-Xno-system-init").hidden().flag();

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  auto mainClassName = geevm::types::convertString(program.get<std::string>("mainclass"));
  std::ranges::replace(mainClassName, u'.', u'/');

  geevm::VmSettings settings;
  if (program["-Xgc-after-every-alloc"] == true) {
    settings.runGcAfterEveryAllocation = true;
  }
  if (program["-Xno-system-init"] == true) {
    settings.noSystemInit = true;
  }

#ifndef NDEBUG
  settings.runGcAfterEveryAllocation = true;
#endif

  auto vm = std::make_unique<geevm::Vm>(settings);
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

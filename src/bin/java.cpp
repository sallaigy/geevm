#include "common/DynamicLibrary.h"
#include "common/Encoding.h"
#include "vm/Thread.h"
#include "vm/Value.h"
#include "vm/Vm.h"

#include <algorithm>
#include <argparse/argparse.hpp>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[])
{
  argparse::ArgumentParser program("java");
  program.add_argument("mainclass");
  program.add_argument("args").remaining().default_value(std::vector<std::string>{});
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

  auto mainClassArg = program.get<std::string>("mainclass");

  auto mainClassName = geevm::utf8ToUtf16(mainClassArg);
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
    std::cerr << "Error: Could not find or load main class " << mainClassArg << std::endl;
    std::cerr << "Caused by: " << geevm::utf16ToUtf8(mainClass.error().exception()) << ": " << geevm::utf16ToUtf8(mainClass.error().message()) << std::endl;
    return 1;
  }

  auto mainMethod = (*mainClass)->getMethod(u"main", u"([Ljava/lang/String;)V");
  if (!mainMethod) {
    std::cerr << "Error: Main method not found in class " << mainClassArg << std::endl;
    return 1;
  }

  auto programArgs = program.get<std::vector<std::string>>("args");

  // As the GC may run during string interning, we need to construct the array in two steps
  auto strArrayCls = vm->resolveClass(u"[Ljava/lang/String;");
  geevm::ScopedGcRootRef<geevm::JavaArray<geevm::Instance*>> argsArray =
      vm->heap().gc().pin(vm->heap().allocateArray<geevm::Instance*>((*strArrayCls)->asArrayClass(), programArgs.size()));

  for (int32_t i = 0; i < programArgs.size(); i++) {
    auto utf16str = geevm::utf8ToUtf16(programArgs[i]);
    geevm::GcRootRef<> handle = vm->heap().intern(utf16str);
    argsArray->setArrayElement(i, handle.get());
  }

  vm->mainThread().start(*mainMethod, {geevm::Value::from<geevm::Instance*>(argsArray.get())});

  return 0;
}

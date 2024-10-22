#include "vm/Vm.h"

#include <algorithm>
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
  vm->initialize();

  auto mainClass = vm->resolveClass(mainClassName);

  if (!mainClass) {
    vm->raiseError(*mainClass.error());
    return 1;
  }

  auto mainMethod = vm->resolveStaticMethod(*mainClass, u"main", u"([Ljava/lang/String;)V");
  if (!mainMethod) {
    // TODO: Raise error
    return 1;
  }

  vm->execute((*mainClass)->asInstanceClass(), mainMethod);

  return 0;
}
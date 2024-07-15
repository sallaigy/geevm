#ifndef GEEVM_VM_INTERPRETER_H
#define GEEVM_VM_INTERPRETER_H

#include <span>
#include <memory>

#include "common/JvmTypes.h"
#include "class_file/Attributes.h"

namespace geevm
{

class Vm;

class Interpreter
{
public:
  virtual void execute(Vm& vm, const Code& code, std::size_t pc) = 0;
  virtual ~Interpreter() = default;
};

std::unique_ptr<Interpreter> createDefaultInterpreter();

}

#endif //GEEVM_VM_INTERPRETER_H

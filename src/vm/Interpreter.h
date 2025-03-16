#ifndef GEEVM_VM_INTERPRETER_H
#define GEEVM_VM_INTERPRETER_H

#include "class_file/Attributes.h"
#include "vm/Frame.h"

#include <memory>
#include <optional>

namespace geevm
{
class JavaThread;
}
namespace geevm
{

class Vm;

class Interpreter
{
public:
  virtual std::optional<Value> execute() = 0;
  virtual ~Interpreter() = default;
};

std::unique_ptr<Interpreter> createDefaultInterpreter(JavaThread& thread);

} // namespace geevm

#endif // GEEVM_VM_INTERPRETER_H

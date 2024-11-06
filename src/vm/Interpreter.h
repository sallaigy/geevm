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
  virtual std::optional<Value> execute(JavaThread& thread, const Code& code, std::size_t pc) = 0;
  virtual ~Interpreter() = default;
};

std::unique_ptr<Interpreter> createDefaultInterpreter();

} // namespace geevm

#endif // GEEVM_VM_INTERPRETER_H

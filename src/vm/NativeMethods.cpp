#include "vm/NativeMethods.h"

using namespace geevm;

std::optional<Value> java_lang_String_intern(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  Value value = args.at(0);

  return std::nullopt;
}

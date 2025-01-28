#ifndef GEEVM_VM_VMUTILS_H
#define GEEVM_VM_VMUTILS_H

#include "common/JvmTypes.h"

namespace geevm
{
class Instance;
class JavaThread;

namespace utils
{

/// Returns the value inside a java.lang.String instance as a JString.
/// This function does not check whether the class is the instance of String; it is the caller's
/// responsability.
types::JString getStringValue(Instance* stringInstance);

/// Creates a new java.lang.String instance on the heap, with the given contents.
Instance* createStringInstance(JavaThread& thread, const types::JString& value);

} // namespace utils
} // namespace geevm

#endif

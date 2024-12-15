#include "vm/JniImplementation.h"

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_jdk_internal_misc_VM_initialize(JNIEnv*, jobject)
{
  // No-op
}
}

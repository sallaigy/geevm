#include "vm/JniImplementation.h"

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_io_FileInputStream_initIDs(JNIEnv*, jobject)
{
  // No-op
}
}

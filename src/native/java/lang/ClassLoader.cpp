#include "vm/JniImplementation.h"

#include <jni.h>

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_lang_ClassLoader_registerNatives(JNIEnv*, jclass)
{
  // No-op for now
}
}

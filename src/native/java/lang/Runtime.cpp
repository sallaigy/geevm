#include "vm/JniImplementation.h"

#include <jni.h>

using namespace geevm;

extern "C"
{

JNIEXPORT jint JNICALL Java_java_lang_Runtime_availableProcessors(JNIEnv*, jclass)
{
  return 1;
}

JNIEXPORT jlong JNICALL Java_java_lang_Runtime_maxMemory(JNIEnv*, jclass)
{
  return 2000;
}
}

#include "vm/JniImplementation.h"

#include <jni.h>
#include <vm/Heap.h>
#include <vm/Thread.h>

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

JNIEXPORT void JNICALL Java_java_lang_Runtime_gc(JNIEnv* env, jobject)
{
  jni::threadFromJniEnv(env).heap().gc().performGarbageCollection();
}
}

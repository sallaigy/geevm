#include <jni.h>

#include "vm/Instance.h"

#include <vm/JniImplementation.h>

extern "C"
{

JNIEXPORT jclass JNICALL Java_java_lang_Object_getClass(JNIEnv* env, jobject obj)
{
  return env->GetObjectClass(obj);
}
}

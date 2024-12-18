#include <jni.h>

#include "vm/Instance.h"

#include <vm/JniImplementation.h>

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_lang_Object_registerNatives(JNIEnv* env, jclass klass)
{
  // No-op
}

JNIEXPORT jclass JNICALL Java_java_lang_Object_getClass(JNIEnv* env, jobject obj)
{
  return env->GetObjectClass(obj);
}

JNIEXPORT jint JNICALL Java_java_lang_Object_hashCode(JNIEnv* env, jobject obj)
{
  auto objectRef = JniTranslate<jobject, Instance*>{}(obj);
  return objectRef->hashCode();
}

JNIEXPORT void JNICALL Java_java_lang_Object_notifyAll(JNIEnv* env, jobject obj)
{
  // Ignored for now
}

JNIEXPORT void JNICALL Java_java_lang_Object_wait__(JNIEnv* env, jobject obj)
{
  // Ignored for now
}

JNIEXPORT void JNICALL Java_java_lang_Object_wait__J(JNIEnv* env, jobject obj, jlong time)
{
  // Ignored for now
}
}

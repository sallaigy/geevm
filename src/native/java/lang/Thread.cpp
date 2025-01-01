#include <jni.h>

#include "vm/Instance.h"

#include <vm/JniImplementation.h>

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_lang_Thread_registerNatives(JNIEnv* env, jclass klass)
{
  // No-op
}

JNIEXPORT jobject JNICALL Java_java_lang_Thread_currentThread(JNIEnv* env, jclass klass)
{
  JavaThread& thread = jni::threadFromJniEnv(env);
  return JniTranslate<Instance*, jobject>{}(thread.instance());
}

JNIEXPORT void JNICALL Java_java_lang_Thread_setPriority0(JNIEnv* env, jobject thread)
{
  // No-op for now.
}

JNIEXPORT jboolean JNICALL Java_java_lang_Thread_isAlive(JNIEnv* env, jobject thread)
{
  jclass klass = env->GetObjectClass(thread);
  auto fieldId = env->GetFieldID(klass, "eetop", "J");
  if (fieldId == nullptr) {
    return JNI_FALSE;
  }

  jlong eetop = env->GetLongField(thread, fieldId);
  if (eetop == 0) {
    return JNI_FALSE;
  }

  return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_java_lang_Thread_start0(JNIEnv* env, jobject thread)
{
  // TODO: We do not have sufficient synchronization to safely start a new thread, so we ignore this call
  //  until we are ready with proper thread support.
}
}

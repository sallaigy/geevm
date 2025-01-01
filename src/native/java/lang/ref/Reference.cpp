#include <jni.h>

extern "C"
{

JNIEXPORT jboolean JNICALL Java_java_lang_ref_Reference_refersTo0(JNIEnv* env, jobject reference, jobject object)
{
  jclass klass = env->GetObjectClass(reference);
  jfieldID field = env->GetFieldID(klass, "referent", "Ljava/lang/Object;");
  if (field == nullptr) {
    return JNI_FALSE;
  }

  jobject referent = env->GetObjectField(reference, field);
  return referent == object;
}
}

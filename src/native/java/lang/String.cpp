#include "common/JvmTypes.h"
#include "vm/JniImplementation.h"
#include "vm/VmUtils.h"

#include <jni.h>
#include <vm/Heap.h>
#include <vm/Thread.h>

using namespace geevm;

extern "C"
{

JNIEXPORT jobject JNICALL Java_java_lang_String_intern(JNIEnv* env, jstring obj)
{
  const jchar* strValue = env->GetStringChars(obj, nullptr);

  JavaThread& thread = jni::threadFromJniEnv(env);

  types::JString str{reinterpret_cast<const char16_t*>(strValue)};
  GcRootRef<> interned = thread.heap().intern(str);

  env->ReleaseStringChars(obj, strValue);
  return jni::translate(interned);
}

JNIEXPORT jobject JNICALL Java_java_lang_StringUTF16_isBigEndian(JNIEnv*, jstring)
{
  return JNI_FALSE;
}
}
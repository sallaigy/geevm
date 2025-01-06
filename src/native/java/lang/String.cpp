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
  auto str = JniTranslate<jobject, GcRootRef<Instance>>{}(obj);
  types::JString strValue = utils::getStringValue(str.get());

  JavaThread& thread = jni::threadFromJniEnv(env);
  Instance* interned = thread.heap().intern(strValue);

  return JniTranslate<GcRootRef<Instance>, jobject>{}(thread.heap().gc().pin(interned));
}

JNIEXPORT jobject JNICALL Java_java_lang_StringUTF16_isBigEndian(JNIEnv*, jstring)
{
  return JNI_FALSE;
}
}
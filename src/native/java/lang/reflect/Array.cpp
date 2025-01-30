#include "vm/JniImplementation.h"

#include <vm/Heap.h>
#include <vm/Thread.h>

using namespace geevm;

extern "C"
{

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Array_newArray(JNIEnv* env, jclass klass, jclass componentType, jint length)
{
  auto classInstance = JniTranslate<jclass, GcRootRef<ClassInstance>>{}(componentType);

  if (classInstance == nullptr) {
    auto npe = env->FindClass("java/lang/NullPointerException");
    env->ThrowNew(npe, "null");
    return nullptr;
  }

  JClass* elementClass = classInstance->target();

  // TODO: Check length
  JavaThread& thread = jni::threadFromJniEnv(env);

  types::JString arrayClassName = u"";
  if (elementClass->isArrayType()) {
    arrayClassName = u"[" + elementClass->className();
  } else {
    arrayClassName = u"[L" + elementClass->className() + u";";
  }

  auto arrayClass = thread.resolveClass(arrayClassName);
  auto newArray = thread.heap().gc().pin(thread.heap().allocateArray((*arrayClass)->asArrayClass(), length)).release();

  return JniTranslate<GcRootRef<ArrayInstance>, jarray>{}(newArray);
}
}

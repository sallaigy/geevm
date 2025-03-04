#include "vm/Class.h"
#include "vm/Heap.h"
#include "vm/Instance.h"
#include "vm/JniImplementation.h"

#include <cstring>

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
  auto objectRef = jni::translate(obj);
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

JNIEXPORT jobject JNICALL Java_java_lang_Object_clone(JNIEnv* env, jobject obj)
{
  auto cloneable = env->FindClass("java/lang/Cloneable");
  if (!env->IsInstanceOf(obj, cloneable)) {
    auto cloneNotSupported = env->FindClass("java/lang/CloneNotSupportedException");
    env->ThrowNew(cloneNotSupported, "class does not implement java.lang.Cloneable");
    return nullptr;
  }

  JavaThread& thread = jni::threadFromJniEnv(env);

  GcRootRef<> objectHandle = jni::translate(obj);

  GcRootRef<> result = nullptr;
  JClass* klass = jni::translate(env->GetObjectClass(obj))->target();
  if (auto instanceClass = klass->asInstanceClass(); instanceClass) {
    auto* newInstance = thread.heap().allocate<ObjectInstance>(instanceClass);
    std::memcpy(newInstance, objectHandle.get(), instanceClass->allocationSize());

    result = thread.heap().gc().pin(newInstance).release();
  } else if (auto arrayClass = klass->asArrayClass(); arrayClass) {
    int32_t length = objectHandle.get()->toArrayInstance()->length();
    ArrayInstance* newInstance = thread.heap().allocateArray(arrayClass, length);
    std::memcpy(newInstance, objectHandle.get(), arrayClass->allocationSize(length));

    result = thread.heap().gc().pin(newInstance).release();
  } else {
    geevm_panic("Unknown class type");
  }

  return jni::translate(result);
}
}

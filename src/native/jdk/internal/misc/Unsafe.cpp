#include "common/JvmTypes.h"
#include "vm/JniImplementation.h"
#include "vm/VmUtils.h"

#include <jni.h>
#include <vm/Heap.h>
#include <vm/Thread.h>

using namespace geevm;

template<class T>
jboolean compareAndSet(GcRootRef<Instance> object, jlong offset, T expected, T desired)
{
  T* target = reinterpret_cast<T*>(reinterpret_cast<char*>(object.get()) + offset);

  std::atomic_ref<T> atomicRef(*target);

  bool success = atomicRef.compare_exchange_strong(expected, desired, std::memory_order_seq_cst);
  return success ? JNI_TRUE : JNI_FALSE;
}

extern "C"
{

JNIEXPORT void JNICALL Java_jdk_internal_misc_Unsafe_registerNatives(JNIEnv*, jclass)
{
  // noop
}

JNIEXPORT jint JNICALL Java_jdk_internal_misc_Unsafe_arrayBaseOffset0(JNIEnv*, jobject, jclass klass)
{
  auto classInstance = jni::translate(klass);
  assert(classInstance->target()->isArrayType());

  const ArrayClass* arrayClass = classInstance->target()->asArrayClass();

  return static_cast<jint>(arrayClass->headerSize());
}

JNIEXPORT jint JNICALL Java_jdk_internal_misc_Unsafe_arrayIndexScale0(JNIEnv*, jobject, jclass arrayClass)
{
  auto klass = jni::translate(arrayClass);

  assert(klass->target()->asArrayClass() != nullptr);

  return klass->target()->asArrayClass()->fieldType().asArrayType()->getElementType().sizeOf();
}

JNIEXPORT jlong JNICALL Java_jdk_internal_misc_Unsafe_objectFieldOffset1(JNIEnv* env, jobject, jclass klass, jstring fieldName)
{
  auto classInstance = jni::translate(klass);

  auto nameStr = env->GetStringChars(fieldName, nullptr);
  auto field = classInstance->target()->lookupFieldByName(reinterpret_cast<const char16_t*>(nameStr));
  env->ReleaseStringChars(fieldName, nameStr);

  if (!field.has_value()) {
    return -1;
  }

  return (*field)->offset();
}

JNIEXPORT void JNICALL Java_jdk_internal_misc_Unsafe_storeFence(JNIEnv*, jobject)
{
  std::atomic_thread_fence(std::memory_order_acquire);
}

JNIEXPORT jboolean JNICALL Java_jdk_internal_misc_Unsafe_compareAndSetInt(JNIEnv*, jobject unsafe, jobject object, jlong offset, jint expected, jint desired)
{
  return compareAndSet<jint>(jni::translate(object), offset, expected, desired);
}

JNIEXPORT jboolean JNICALL Java_jdk_internal_misc_Unsafe_compareAndSetLong(JNIEnv*, jobject unsafe, jobject object, jlong offset, jlong expected, jlong desired)
{
  return compareAndSet<jlong>(jni::translate(object), offset, expected, desired);
}

JNIEXPORT jboolean JNICALL Java_jdk_internal_misc_Unsafe_compareAndSetReference(JNIEnv*, jobject unsafe, jobject object, jlong offset, jobject expected,
                                                                                jobject desired)
{
  return compareAndSet<Instance*>(jni::translate(object), offset, jni::translate(expected).get(),
                                  jni::translate(desired).get());
}

JNIEXPORT jobject JNICALL Java_jdk_internal_misc_Unsafe_getReferenceVolatile(JNIEnv* env, jobject unsafe, jobject object, jlong offset)
{
  GcRootRef<Instance> instance = jni::translate(object);

  Instance** target = reinterpret_cast<Instance**>(reinterpret_cast<char*>(instance.get()) + offset);
  std::atomic_ref<Instance**> atomicRef(target);
  auto loaded = jni::threadFromJniEnv(env).heap().gc().pin(*atomicRef.load()).release();

  return jni::translate(loaded);
}

JNIEXPORT jint JNICALL Java_jdk_internal_misc_Unsafe_getIntVolatile(JNIEnv*, jobject unsafe, jobject object, jlong offset)
{
  GcRootRef<Instance> instance = jni::translate(object);

  int32_t* target = reinterpret_cast<int32_t*>(reinterpret_cast<char*>(instance.get()) + offset);
  std::atomic_ref<int32_t*> atomicRef(target);

  return *atomicRef.load();
}
}

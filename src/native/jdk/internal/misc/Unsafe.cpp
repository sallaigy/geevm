#include "common/JvmTypes.h"
#include "vm/JniImplementation.h"
#include "vm/VmUtils.h"

#include <jni.h>
#include <vm/Heap.h>
#include <vm/Thread.h>

using namespace geevm;

template<class T>
jboolean compareAndSet(Instance* object, jlong offset, T expected, T desired)
{
  T* target = reinterpret_cast<T*>(reinterpret_cast<char*>(object) + offset);

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
  auto classInstance = JniTranslate<jclass, ClassInstance*>{}(klass);
  assert(classInstance->target()->isArrayType());

  const ArrayClass* arrayClass = classInstance->target()->asArrayClass();

  return static_cast<jint>(arrayClass->headerSize());
}

JNIEXPORT jint JNICALL Java_jdk_internal_misc_Unsafe_arrayIndexScale0(JNIEnv*, jobject, jclass)
{
  return sizeof(Value);
}

JNIEXPORT jlong JNICALL Java_jdk_internal_misc_Unsafe_objectFieldOffset1(JNIEnv* env, jobject theUnsafe, jclass klass, jstring fieldName)
{
  auto classInstance = JniTranslate<jclass, ClassInstance*>{}(klass);
  auto nameObj = JniTranslate<jobject, Instance*>{}(fieldName);

  auto nameStr = utils::getStringValue(nameObj);
  auto field = classInstance->target()->lookupFieldByName(nameStr);

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
  return compareAndSet<jint>(JniTranslate<jobject, Instance*>{}(object), offset, expected, desired);
}

JNIEXPORT jboolean JNICALL Java_jdk_internal_misc_Unsafe_compareAndSetLong(JNIEnv*, jobject unsafe, jobject object, jlong offset, jlong expected, jlong desired)
{
  return compareAndSet<jlong>(JniTranslate<jobject, Instance*>{}(object), offset, expected, desired);
}

JNIEXPORT jboolean JNICALL Java_jdk_internal_misc_Unsafe_compareAndSetReference(JNIEnv*, jobject unsafe, jobject object, jlong offset, jobject expected,
                                                                                jobject desired)
{
  return compareAndSet<Instance*>(JniTranslate<jobject, Instance*>{}(object), offset, JniTranslate<jobject, Instance*>{}(expected),
                                  JniTranslate<jobject, Instance*>{}(desired));
}

JNIEXPORT jobject JNICALL Java_jdk_internal_misc_Unsafe_getReferenceVolatile(JNIEnv*, jobject unsafe, jobject object, jlong offset)
{
  Instance* instance = JniTranslate<jobject, Instance*>{}(object);

  Instance** target = reinterpret_cast<Instance**>(reinterpret_cast<char*>(instance) + offset);
  std::atomic_ref<Instance**> atomicRef(target);

  return JniTranslate<Instance*, jobject>{}(*atomicRef.load());
}

JNIEXPORT jint JNICALL Java_jdk_internal_misc_Unsafe_getIntVolatile(JNIEnv*, jobject unsafe, jobject object, jlong offset)
{
  Instance* instance = JniTranslate<jobject, Instance*>{}(object);

  int32_t* target = reinterpret_cast<int32_t*>(reinterpret_cast<char*>(instance) + offset);
  std::atomic_ref<int32_t*> atomicRef(target);

  return *atomicRef.load();
}
}

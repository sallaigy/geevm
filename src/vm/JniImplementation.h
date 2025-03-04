#ifndef GEEVM_JNIIMPLEMENTATION_H
#define GEEVM_JNIIMPLEMENTATION_H

#include "vm/GarbageCollector.h"
#include "vm/Instance.h"

#include <bit>
#include <jni.h>

namespace geevm
{

class Instance;
class ClassInstance;
class ArrayInstance;
class JavaThread;
class JField;
class JMethod;

namespace jni
{

template<class From, class To>
struct JniTranslateImpl
{
  To operator()(From from) const
  {
    return std::bit_cast<To>(from);
  }
};

template<class From>
struct JniMirror;

#define GEEVM_JNI_TRANSLATE(TYPE1, TYPE2) \
  template<>                              \
  struct JniMirror<TYPE1>                 \
  {                                       \
    using MirrorTy = TYPE2;               \
  };                                      \
  template<>                              \
  struct JniMirror<TYPE2>                 \
  {                                       \
    using MirrorTy = TYPE1;               \
  };

GEEVM_JNI_TRANSLATE(jobject, GcRootRef<Instance>)
GEEVM_JNI_TRANSLATE(jclass, GcRootRef<ClassInstance>)
GEEVM_JNI_TRANSLATE(jarray, GcRootRef<ArrayInstance>)
GEEVM_JNI_TRANSLATE(jthrowable, GcRootRef<JavaThrowable>)
GEEVM_JNI_TRANSLATE(jstring, GcRootRef<JavaString>)
GEEVM_JNI_TRANSLATE(jfieldID, JField*)
GEEVM_JNI_TRANSLATE(jmethodID, JMethod*)
GEEVM_JNI_TRANSLATE(jbyteArray, GcRootRef<JavaArray<int8_t>>)
GEEVM_JNI_TRANSLATE(jobjectArray, GcRootRef<JavaArray<Instance*>>)

/// Retrieves the current Java thread from the env instance.
JavaThread& threadFromJniEnv(JNIEnv* env);

/// Translates between internal JVM types and JNI types.
///
/// For example, `translate(jclass)` returns `GcRootRef<ClassInstance>`,
/// `translate(GcRootRef<ClassInstance>` a `jclass` object.
template<class T, class R = typename JniMirror<std::remove_reference_t<T>>::MirrorTy>
R translate(T from)
{
  return JniTranslateImpl<T, R>{}(from);
}

} // namespace jni

class JniImplementation
{
public:
  JniImplementation(JavaThread& thread);

  JNIEnv_* getEnv();

private:
  JavaThread& mThread;
  JNIEnv mEnv;
  JNINativeInterface_ mFunctions;
};

} // namespace geevm

#endif // GEEVM_JNIIMPLEMENTATION_H

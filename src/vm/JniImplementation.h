#ifndef GEEVM_JNIIMPLEMENTATION_H
#define GEEVM_JNIIMPLEMENTATION_H

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

template<class From, class To>
struct JniTranslateImpl
{
  To operator()(From from) const
  {
    return std::bit_cast<To>(from);
  }
};

template<class From, class To>
struct JniTranslate;

#define GEEVM_JNI_TRANSLATE(TYPE1, TYPE2)                            \
  template<>                                                         \
  struct JniTranslate<TYPE1, TYPE2> : JniTranslateImpl<TYPE1, TYPE2> \
  {                                                                  \
  };                                                                 \
  template<>                                                         \
  struct JniTranslate<TYPE2, TYPE1> : JniTranslateImpl<TYPE2, TYPE1> \
  {                                                                  \
  };

GEEVM_JNI_TRANSLATE(jobject, Instance*)
GEEVM_JNI_TRANSLATE(jclass, ClassInstance*)
GEEVM_JNI_TRANSLATE(jarray, ArrayInstance*)
GEEVM_JNI_TRANSLATE(jthrowable, Instance*)
GEEVM_JNI_TRANSLATE(jstring, Instance*)
GEEVM_JNI_TRANSLATE(jfieldID, JField*)
GEEVM_JNI_TRANSLATE(jmethodID, JMethod*)
GEEVM_JNI_TRANSLATE(jbyteArray, ArrayInstance*)
GEEVM_JNI_TRANSLATE(jobjectArray, ArrayInstance*)

namespace jni
{

JavaThread& threadFromJniEnv(JNIEnv* env);

}

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

#include "vm/Instance.h"
#include "vm/JniImplementation.h"

#include <vm/Thread.h>

using namespace geevm;

extern "C"
{

JNIEXPORT jobject JNICALL Java_java_lang_Throwable_fillInStackTrace(JNIEnv* env, jobject throwable, jint depth)
{
  JavaThread& thread = jni::threadFromJniEnv(env);
  auto array = thread.createStackTrace();

  auto exceptionInstance = JniTranslate<jobject, Instance*>{}(throwable);
  exceptionInstance->setFieldValue<Instance*>(u"stackTrace", u"[Ljava/lang/StackTraceElement;", nullptr);
  exceptionInstance->setFieldValue<Instance*>(u"backtrace", u"Ljava/lang/Object;", array);
  exceptionInstance->setFieldValue<int32_t>(u"depth", u"I", array->asArrayInstance()->length());

  return throwable;
}

JNIEXPORT jint JNICALL Java_java_lang_Throwable_getStackTraceDepth(JNIEnv* env, jobject throwable)
{
  auto exceptionInstance = JniTranslate<jobject, Instance*>{}(throwable);
  auto backtrace = exceptionInstance->getFieldValue<Instance*>(u"backtrace", u"Ljava/lang/Object;");

  if (backtrace == nullptr) {
    return 0;
  }

  return backtrace->asArrayInstance()->length();
}

JNIEXPORT jobject JNICALL Java_java_lang_Throwable_getStackTraceElement(JNIEnv* env, jobject throwable, jint index)
{
  auto exceptionInstance = JniTranslate<jobject, Instance*>{}(throwable);
  auto backtrace = exceptionInstance->getFieldValue<Instance*>(u"backtrace", u"Ljava/lang/Object;");
  auto elem = backtrace->asArray<Instance*>()->getArrayElement(index);

  assert(elem.has_value());

  return JniTranslate<Instance*, jobject>{}(*elem);
}
}

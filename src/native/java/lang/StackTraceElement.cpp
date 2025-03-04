#include "vm/Instance.h"
#include "vm/JniImplementation.h"

#include <vm/Thread.h>

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_lang_StackTraceElement_initStackTraceElements(JNIEnv* env, jclass klass, jobjectArray elements, jthrowable throwable)
{
  auto stackTraceArray = jni::translate(elements);
  GcRootRef<JavaThrowable> throwableInstance = jni::translate(throwable);
  auto storedBackTrace = throwableInstance->getFieldValue<Instance*>(u"backtrace", u"Ljava/lang/Object;")->toArray<Instance*>();

  for (int32_t i = 0; i < stackTraceArray->length(); i++) {
    stackTraceArray->setArrayElement(i, *storedBackTrace->getArrayElement(i));
  }
}
}

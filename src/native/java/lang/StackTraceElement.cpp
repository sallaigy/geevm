#include "vm/Instance.h"
#include "vm/JniImplementation.h"

#include <vm/Thread.h>

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_lang_StackTraceElement_initStackTraceElements(JNIEnv* env, jclass klass, jobjectArray elements, jthrowable throwable)
{
  ArrayInstance* stackTraceArray = JniTranslate<jobjectArray, ArrayInstance*>{}(elements);
  Instance* throwableInstance = JniTranslate<jthrowable, Instance*>{}(throwable);
  auto storedBackTrace = throwableInstance->getFieldValue<Instance*>(u"backtrace", u"Ljava/lang/Object;")->asArrayInstance();

  for (int32_t i = 0; i < stackTraceArray->length(); i++) {
    stackTraceArray->setArrayElement(i, *storedBackTrace->getArrayElement(i));
  }
}
}

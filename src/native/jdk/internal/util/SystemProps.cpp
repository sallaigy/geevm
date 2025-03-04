#include "common/Encoding.h"
#include "vm/Heap.h"
#include "vm/JniImplementation.h"
#include "vm/Thread.h"
#include "vm/Vm.h"

#include <filesystem>

using namespace geevm;

extern "C"
{
JNIEXPORT jobjectArray JNICALL Java_jdk_internal_util_SystemProps_00024Raw_platformProperties(JNIEnv* env, jclass klass)
{
  jclass strArrayClass = env->FindClass("[Ljava/lang/String;");
  if (strArrayClass == nullptr) {
    return nullptr;
  }

  auto lengthField = env->GetFieldID(klass, "FIXED_LENGTH", "I");
  if (lengthField == nullptr) {
    return nullptr;
  }

  jint arrayLength = env->GetStaticIntField(klass, lengthField);

  namespace fs = std::filesystem;
  auto temp = fs::temp_directory_path().u16string();

  JavaThread& thread = jni::threadFromJniEnv(env);

  auto targetClass = jni::translate(strArrayClass)->target();

  auto* allocatedArray = thread.heap().allocateArray<Instance*>(targetClass->asArrayClass(), arrayLength);
  GcRootRef<JavaArray<Instance*>> propsArray = thread.heap().gc().pin(allocatedArray).release();
  propsArray->setArrayElement(18, thread.heap().intern(temp).get());
  propsArray->setArrayElement(36, thread.heap().intern(temp).get());
  propsArray->setArrayElement(37, thread.heap().intern(temp).get());
  propsArray->setArrayElement(38, thread.heap().intern(u"user").get());
  propsArray->setArrayElement(4, thread.heap().intern(u"UTF-8").get());
  propsArray->setArrayElement(19, thread.heap().intern(u"\n").get());
  propsArray->setArrayElement(5, thread.heap().intern(u"/").get());
  propsArray->setArrayElement(23, thread.heap().intern(u":").get());

  return jni::translate(propsArray);
}

JNIEXPORT jobjectArray JNICALL Java_jdk_internal_util_SystemProps_00024Raw_vmProperties(JNIEnv* env, jclass klass)
{
  jclass strArrayClass = env->FindClass("[Ljava/lang/String;");
  if (strArrayClass == nullptr) {
    return nullptr;
  }
  int32_t arrayLength = 2;

  JavaThread& thread = jni::threadFromJniEnv(env);

  auto targetClass = jni::translate(strArrayClass)->target();

  GcRootRef<JavaArray<Instance*>> propsArray =
      thread.heap().gc().pin(thread.heap().allocateArray<Instance*>(targetClass->asArrayClass(), arrayLength)).release();
  propsArray->setArrayElement(0, thread.heap().intern(u"java.home").get());
  propsArray->setArrayElement(1, thread.heap().intern(utf8ToUtf16(thread.vm().settings().javaHome)).get());

  return jni::translate(propsArray);
}
}

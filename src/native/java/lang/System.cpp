#include <jni.h>

#include "vm/Instance.h"

#include <vm/JniImplementation.h>

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_lang_System_registerNatives()
{
}

JNIEXPORT jobject JNICALL Java_java_lang_System_initProperties(JNIEnv* env, jobject obj)
{
  return obj;
}

JNIEXPORT jlong JNICALL Java_java_lang_System_nanoTime(JNIEnv* env, jclass klass)
{
  auto now = std::chrono::high_resolution_clock::now();
  auto duration = now.time_since_epoch();

  return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

JNIEXPORT void JNICALL Java_java_lang_System_arraycopy(JNIEnv* env, jclass klass, jobject src, jint srcOffset, jobject dst, jint dstOffset, jint length)
{
  auto sourceArray = JniTranslate<jobject, Instance*>{}(src)->asArrayInstance();
  auto targetArray = JniTranslate<jobject, Instance*>{}(dst)->asArrayInstance();
  assert(srcOffset < sourceArray->length());
  assert(dst != nullptr);

  for (int32_t i = 0; i < length; i++) {
    Value value = *sourceArray->getArrayElement(srcOffset + i);
    targetArray->setArrayElement(dstOffset + i, value);
  }
}

JNIEXPORT void JNICALL Java_java_lang_System_setIn0(JNIEnv* env, jclass klass, jobject stream)
{
  auto fieldId = env->GetFieldID(klass, "in", "Ljava/io/InputStream;");
  if (fieldId == nullptr) {
    return;
  }
  env->SetStaticObjectField(klass, fieldId, stream);
}

JNIEXPORT void JNICALL Java_java_lang_System_setOut0(JNIEnv* env, jclass klass, jobject stream)
{
  auto fieldId = env->GetFieldID(klass, "out", "Ljava/io/PrintStream;");
  if (fieldId == nullptr) {
    return;
  }
  env->SetStaticObjectField(klass, fieldId, stream);
}

JNIEXPORT void JNICALL Java_java_lang_System_setErr0(JNIEnv* env, jclass klass, jobject stream)
{
  auto fieldId = env->GetFieldID(klass, "out", "Ljava/io/PrintStream;");
  if (fieldId == nullptr) {
    return;
  }
  env->SetStaticObjectField(klass, fieldId, stream);
}
}

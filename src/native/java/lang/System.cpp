#include <jni.h>

#include "vm/Instance.h"

#include <vm/JniImplementation.h>

#include <cstring>

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

JNIEXPORT void JNICALL Java_java_lang_System_arraycopy(JNIEnv* env, jclass klass, jobject src, jint srcPos, jobject dest, jint destPos, jint length)
{
  JavaThread& thread = jni::threadFromJniEnv(env);

  if (src == nullptr || dest == nullptr) {
    thread.throwException(u"java/lang/NullPointerException");
    return;
  }

  auto sourceArray = JniTranslate<jobject, Instance*>{}(src)->asArrayInstance();
  auto targetArray = JniTranslate<jobject, Instance*>{}(dest)->asArrayInstance();

  if (sourceArray == nullptr || targetArray == nullptr) {
    thread.throwException(u"java/lang/ArrayStoreException", u"not an array instance");
    return;
  }

  auto sourceElementTy = sourceArray->getClass()->asArrayClass()->fieldType().asArrayType()->getElementType();
  auto targetElementTy = sourceArray->getClass()->asArrayClass()->fieldType().asArrayType()->getElementType();

  if (srcPos < 0 || destPos < 0 || length < 0) {
    thread.throwException(u"java/lang/IndexOutOfBoundsException");
    return;
  }

  if (srcPos + length > sourceArray->length()) {
    thread.throwException(u"java/lang/IndexOutOfBoundsException", u"source array range is out of bounds");
    return;
  }

  if (destPos + length > targetArray->length()) {
    thread.throwException(u"java/lang/IndexOutOfBoundsException", u"dest array range is out of bounds");
    return;
  }

  if (auto sourcePrimitive = sourceElementTy.asPrimitive(); sourcePrimitive) {
    if (auto targetPrimitive = targetElementTy.asPrimitive(); targetPrimitive) {
      if (*sourcePrimitive != *targetPrimitive) {
        thread.throwException(u"java/lang/ArrayStoreException", u"source and target arrays are of different type");
        return;
      }

      size_t elemSize = sourceElementTy.sizeOf();

      auto srcBegin = reinterpret_cast<char*>(sourceArray->fieldsStart()) + elemSize * srcPos;
      auto dstBegin = reinterpret_cast<char*>(targetArray->fieldsStart()) + elemSize * destPos;

      std::memmove(dstBegin, srcBegin, elemSize * length);
    } else {
      thread.throwException(u"java/lang/ArrayStoreException", u"source and target arrays are of different type");
    }
  } else if (sourceElementTy.asReference().has_value() && targetElementTy.asReference().has_value()) {
    JavaArray<Instance*>* sourceRefArray = sourceArray->asArray<Instance*>();
    JavaArray<Instance*>* targetRefArray = targetArray->asArray<Instance*>();
    for (int32_t i = 0; i < length; i++) {
      Instance* value = *sourceRefArray->getArrayElement(srcPos + i);
      targetRefArray->setArrayElement(destPos + i, value);
    }
  } else {
    thread.throwException(u"java/lang/ArrayStoreException", u"source and target arrays are of different type");
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
  auto fieldId = env->GetFieldID(klass, "err", "Ljava/io/PrintStream;");
  if (fieldId == nullptr) {
    return;
  }
  env->SetStaticObjectField(klass, fieldId, stream);
}
}

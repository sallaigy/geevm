#include "vm/Class.h"
#include "vm/Instance.h"
#include "vm/JniImplementation.h"
#include "vm/Thread.h"

#include <chrono>
#include <cstring>
#include <format>

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
    thread.throwException(u"java/lang/NullPointerException", u"null");
    return;
  }

  auto sourceArray = JniTranslate<jobject, GcRootRef<Instance>>{}(src)->toArrayInstance();
  auto targetArray = JniTranslate<jobject, GcRootRef<Instance>>{}(dest)->toArrayInstance();

  if (sourceArray == nullptr || targetArray == nullptr) {
    thread.throwException(u"java/lang/ArrayStoreException", u"not an array instance");
    return;
  }

  auto sourceElementTy = sourceArray->getClass()->asArrayClass()->fieldType().asArrayType()->getElementType();
  auto targetElementTy = sourceArray->getClass()->asArrayClass()->fieldType().asArrayType()->getElementType();

  if (srcPos < 0) {
    auto message = std::format("arraycopy: source index {} out of bounds for {}[{}]", srcPos, types::convertJString(sourceElementTy.toJavaString()),
                               sourceArray->length());
    thread.throwException(u"java/lang/IndexOutOfBoundsException", types::convertString(message));
    return;
  }

  if (destPos < 0) {
    auto message = std::format("arraycopy: destination index {} out of bounds for {}[{}]", destPos, types::convertJString(targetElementTy.toJavaString()),
                               targetArray->length());
    thread.throwException(u"java/lang/IndexOutOfBoundsException", types::convertString(message));
    return;
  }

  if (srcPos + length > sourceArray->length()) {
    auto message = std::format("arraycopy: last source index {} out of bounds for {}[{}]", srcPos + length,
                               types::convertJString(sourceElementTy.toJavaString()), sourceArray->length());
    thread.throwException(u"java/lang/IndexOutOfBoundsException", types::convertString(message));
    return;
  }

  if (destPos + length > targetArray->length()) {
    auto message = std::format("arraycopy: last destination index {} out of bounds for {}[{}]", destPos + length,
                               types::convertJString(targetElementTy.toJavaString()), targetArray->length());
    thread.throwException(u"java/lang/IndexOutOfBoundsException", types::convertString(message));
    return;
  }

  if (length < 0) {
    auto message = std::format("arraycopy: length {} is negative", length);
    thread.throwException(u"java/lang/IndexOutOfBoundsException", types::convertString(message));
    return;
  }

  if (auto sourcePrimitive = sourceElementTy.asPrimitive(); sourcePrimitive) {
    if (auto targetPrimitive = targetElementTy.asPrimitive(); targetPrimitive) {
      if (*sourcePrimitive != *targetPrimitive) {
        thread.throwException(u"java/lang/ArrayStoreException", u"source and target arrays are of different type");
        return;
      }

      size_t elemSize = sourceElementTy.sizeOf();

      auto srcBegin = reinterpret_cast<char*>(sourceArray->elementsStart()) + elemSize * srcPos;
      auto dstBegin = reinterpret_cast<char*>(targetArray->elementsStart()) + elemSize * destPos;

      std::memmove(dstBegin, srcBegin, elemSize * length);
    } else {
      thread.throwException(u"java/lang/ArrayStoreException", u"source and target arrays are of different type");
    }
  } else if (sourceElementTy.asReference().has_value() && targetElementTy.asReference().has_value()) {
    JavaArray<Instance*>* sourceRefArray = sourceArray->toArray<Instance*>();
    JavaArray<Instance*>* targetRefArray = targetArray->toArray<Instance*>();
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

JNIEXPORT jint JNICALL Java_java_lang_System_identityHashCode(JNIEnv*, jclass, jobject object)
{
  return JniTranslate<jobject, GcRootRef<Instance>>{}(object)->hashCode();
}
}

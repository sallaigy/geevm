#include "vm/JniImplementation.h"
#include "common/Encoding.h"
#include "vm/Class.h"
#include "vm/Field.h"
#include "vm/Heap.h"
#include "vm/Instance.h"
#include "vm/Thread.h"
#include "vm/VmUtils.h"

#include <cstring>
#include <jni.h>

using namespace geevm;

static void initializeFunctions(JNINativeInterface_& functions);

JniImplementation::JniImplementation(JavaThread& thread)
  : mThread(thread), mEnv(), mFunctions()
{
  mFunctions.reserved0 = static_cast<void*>(&mThread);

  initializeFunctions(mFunctions);

  mEnv.functions = &mFunctions;
}

JNIEnv* JniImplementation::getEnv()
{
  return &mEnv;
}

JavaThread& jni::threadFromJniEnv(JNIEnv* env)
{
  JavaThread* thread = static_cast<JavaThread*>(env->functions->reserved0);
  assert(thread != nullptr);

  return *thread;
}

template<class T>
static T getField(JNIEnv* env, jobject object, jfieldID field)
{
  auto objectInstance = jni::translate(object);
  auto fieldOffset = jni::translate(field)->offset();

  return objectInstance->getFieldValue<T>(fieldOffset);
}

template<class T>
static T getStaticField(JNIEnv* env, jclass klass, jfieldID field)
{
  auto javaClass = jni::translate(klass)->target();
  auto fieldOffset = jni::translate(field)->offset();

  return javaClass->getStaticFieldValue<T>(fieldOffset);
}

void initializeFunctions(JNINativeInterface_& functions)
{
  /// Returns the version of the native method interface.
  functions.GetVersion = [](JNIEnv* env) {
    return JNI_VERSION_10;
  };

  /// Returns the class of an object.
  functions.GetObjectClass = [](JNIEnv* env, jobject obj) -> jclass {
    GcRootRef<Instance> objectRef = jni::translate(obj);
    return jni::translate(objectRef->getClass()->classInstance());
  };

  functions.FindClass = [](JNIEnv* env, const char* name) -> jclass {
    auto nameUtf16 = utf8ToUtf16(std::string{name});
    auto klass = jni::threadFromJniEnv(env).resolveClass(nameUtf16);
    if (!klass) {
      jni::threadFromJniEnv(env).throwException(klass.error().exception(), klass.error().message());
      return nullptr;
    }

    return jni::translate((*klass)->classInstance());
  };

  functions.GetFieldID = [](JNIEnv* env, jclass klass, const char* name, const char* sig) -> jfieldID {
    JavaThread& thread = jni::threadFromJniEnv(env);
    auto clsInstance = jni::translate(klass);

    clsInstance->target()->initialize(thread);
    auto field = clsInstance->target()->lookupField(utf8ToUtf16(name), utf8ToUtf16(sig));
    if (!field) {
      thread.throwException(u"java/lang/NoSuchFieldError");
      return nullptr;
    }

    return jni::translate(*field);
  };

  functions.GetMethodID = [](JNIEnv* env, jclass klass, const char* name, const char* sig) -> jmethodID {
    JavaThread& thread = jni::threadFromJniEnv(env);
    auto clsInstance = jni::translate(klass);
    clsInstance->target()->initialize(thread);

    auto method = clsInstance->target()->getVirtualMethod(utf8ToUtf16(name), utf8ToUtf16(sig));
    if (!method) {
      thread.throwException(u"java/lang/NoSuchMethodError");
      return nullptr;
    }

    return jni::translate(*method);
  };

  functions.SetStaticObjectField = [](JNIEnv* env, jclass klass, jfieldID field, jobject value) -> void {
    auto clsInstance = jni::translate(klass);
    auto fieldOffset = jni::translate(field)->offset();
    auto translatedValue = jni::translate(value);

    clsInstance->target()->setStaticFieldValue(fieldOffset, Value::from<Instance*>(translatedValue.get()));
  };

  functions.GetLongField = getField<int64_t>;
  functions.GetIntField = getField<int32_t>;
  functions.GetObjectField = [](JNIEnv* env, jobject object, jfieldID field) {
    auto objectInstance = jni::translate(object);
    auto fieldOffset = jni::translate(field)->offset();

    GcRootRef<> ret = jni::threadFromJniEnv(env).addJniHandle(objectInstance->getFieldValue<Instance*>(fieldOffset));
    return jni::translate(ret);
  };

  functions.GetStaticIntField = getStaticField<int32_t>;

  functions.GetStringChars = [](JNIEnv*, jstring string, jboolean* isCopy) -> const jchar* {
    auto stringInstance = jni::translate(string);
    types::JString value = utils::getStringValue(stringInstance.get());

    jchar* buffer = new jchar[value.size() + 1];
    for (size_t i = 0; i < value.size(); ++i) {
      buffer[i] = value[i];
    }
    buffer[value.size()] = 0;

    if (isCopy != nullptr) {
      *isCopy = JNI_TRUE;
    }

    return buffer;
  };

  functions.ReleaseStringChars = [](JNIEnv* env, jstring string, const jchar* chars) -> void {
    delete[] chars;
  };

  functions.GetStringUTFChars = [](JNIEnv*, jstring string, jboolean* isCopy) -> const char* {
    auto stringInstance = jni::translate(string);
    types::JString value = utils::getStringValue(stringInstance.get());
    std::string utf8 = utf16ToUtf8(value);

    char* buffer = new char[value.size() + 1];
    std::memcpy(buffer, utf8.c_str(), value.size() + 1);

    if (isCopy != nullptr) {
      *isCopy = JNI_TRUE;
    }

    return buffer;
  };

  functions.ReleaseStringUTFChars = [](JNIEnv*, jstring, const char* chars) -> void {
    delete[] chars;
  };

  functions.ThrowNew = [](JNIEnv* env, jclass klass, const char* message) -> jint {
    JavaThread& thread = jni::threadFromJniEnv(env);
    auto clsInstance = jni::translate(klass);

    thread.throwException(clsInstance->target()->className(), u"null");

    return 0;
  };

  functions.IsInstanceOf = [](JNIEnv* env, jobject object, jclass klass) -> jboolean {
    auto objectClass = env->GetObjectClass(object);
    auto objectClassInstance = jni::translate(objectClass);
    auto clsInstance = jni::translate(klass);
    // TODO: Null check
    return objectClassInstance->target()->isInstanceOf(clsInstance->target());
  };
}

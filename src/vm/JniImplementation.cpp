#include "vm/JniImplementation.h"

#include "Heap.h"
#include "vm/Class.h"
#include "vm/Field.h"
#include "vm/Instance.h"
#include "vm/Thread.h"

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
  auto objectInstance = JniTranslate<jobject, GcRootRef<Instance>>{}(object);
  auto fieldOffset = JniTranslate<jfieldID, JField*>{}(field)->offset();

  return objectInstance->getFieldValue<T>(fieldOffset);
}

template<class T>
static T getStaticField(JNIEnv* env, jclass klass, jfieldID field)
{
  auto javaClass = JniTranslate<jclass, GcRootRef<ClassInstance>>{}(klass)->target();
  auto fieldOffset = JniTranslate<jfieldID, JField*>{}(field)->offset();

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
    GcRootRef<Instance> objectRef = JniTranslate<jobject, GcRootRef<Instance>>{}(obj);
    return JniTranslate<GcRootRef<ClassInstance>, jclass>{}(objectRef->getClass()->classInstance());
  };

  functions.FindClass = [](JNIEnv* env, const char* name) -> jclass {
    auto nameUtf16 = types::convertString(std::string{name});
    auto klass = jni::threadFromJniEnv(env).resolveClass(nameUtf16);
    if (!klass) {
      jni::threadFromJniEnv(env).throwException(klass.error().exception(), klass.error().message());
      return nullptr;
    }

    return JniTranslate<GcRootRef<ClassInstance>, jclass>{}((*klass)->classInstance());
  };

  functions.GetFieldID = [](JNIEnv* env, jclass klass, const char* name, const char* sig) -> jfieldID {
    JavaThread& thread = jni::threadFromJniEnv(env);
    auto clsInstance = JniTranslate<jclass, GcRootRef<ClassInstance>>{}(klass);

    clsInstance->target()->initialize(thread);
    auto field = clsInstance->target()->lookupField(types::convertString(name), types::convertString(sig));
    if (!field) {
      thread.throwException(u"java/lang/NoSuchFieldError");
      return nullptr;
    }

    return JniTranslate<JField*, jfieldID>{}(*field);
  };

  functions.GetMethodID = [](JNIEnv* env, jclass klass, const char* name, const char* sig) -> jmethodID {
    JavaThread& thread = jni::threadFromJniEnv(env);
    auto clsInstance = JniTranslate<jclass, GcRootRef<ClassInstance>>{}(klass);
    clsInstance->target()->initialize(thread);

    auto method = clsInstance->target()->getVirtualMethod(types::convertString(name), types::convertString(sig));
    if (!method) {
      thread.throwException(u"java/lang/NoSuchMethodError");
      return nullptr;
    }

    return JniTranslate<JMethod*, jmethodID>{}(*method);
  };

  functions.SetStaticObjectField = [](JNIEnv* env, jclass klass, jfieldID field, jobject value) -> void {
    auto clsInstance = JniTranslate<jclass, GcRootRef<ClassInstance>>{}(klass);
    auto fieldOffset = JniTranslate<jfieldID, JField*>{}(field)->offset();
    auto translatedValue = JniTranslate<jobject, GcRootRef<Instance>>{}(value);

    clsInstance->target()->setStaticFieldValue(fieldOffset, Value::from<Instance*>(translatedValue.get()));
  };

  functions.GetLongField = getField<int64_t>;
  functions.GetIntField = getField<int32_t>;
  functions.GetObjectField = [](JNIEnv* env, jobject object, jfieldID field) {
    auto objectInstance = JniTranslate<jobject, GcRootRef<Instance>>{}(object);
    auto fieldOffset = JniTranslate<jfieldID, JField*>{}(field)->offset();

    GcRootRef<Instance> ret = jni::threadFromJniEnv(env).heap().gc().pin(objectInstance->getFieldValue<Instance*>(fieldOffset));
    return JniTranslate<GcRootRef<Instance>, jobject>{}(ret);
  };

  functions.GetStaticIntField = getStaticField<int32_t>;
}

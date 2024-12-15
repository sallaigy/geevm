#include "JniImplementation.h"

#include "Instance.h"

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
  auto objectInstance = JniTranslate<jobject, Instance*>{}(object);
  auto fieldOffset = JniTranslate<jfieldID, JField*>{}(field)->offset();

  return objectInstance->getFieldValue<T>(fieldOffset);
}

void initializeFunctions(JNINativeInterface_& functions)
{
  /// Returns the version of the native method interface.
  functions.GetVersion = [](JNIEnv* env) {
    return JNI_VERSION_10;
  };

  /// Returns the class of an object.
  functions.GetObjectClass = [](JNIEnv* env, jobject obj) -> jclass {
    Instance* objectRef = JniTranslate<jobject, Instance*>{}(obj);
    return JniTranslate<ClassInstance*, jclass>{}(objectRef->getClass()->classInstance()->asClassInstance());
  };

  functions.FindClass = [](JNIEnv* env, const char* name) -> jclass {
    auto nameUtf16 = types::convertString(std::string{name});
    auto klass = jni::threadFromJniEnv(env).resolveClass(nameUtf16);
    if (!klass) {
      jni::threadFromJniEnv(env).throwException(klass.error().exception(), klass.error().message());
      return nullptr;
    }

    return JniTranslate<ClassInstance*, jclass>{}((*klass)->classInstance()->asClassInstance());
  };

  functions.GetFieldID = [](JNIEnv* env, jclass klass, const char* name, const char* sig) -> jfieldID {
    JavaThread& thread = jni::threadFromJniEnv(env);
    auto clsInstance = JniTranslate<jclass, ClassInstance*>{}(klass);

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
    auto clsInstance = JniTranslate<jclass, ClassInstance*>{}(klass);
    clsInstance->target()->initialize(thread);

    auto method = clsInstance->target()->getVirtualMethod(types::convertString(name), types::convertString(sig));
    if (!method) {
      thread.throwException(u"java/lang/NoSuchMethodError");
      return nullptr;
    }

    return JniTranslate<JMethod*, jmethodID>{}(*method);
  };

  functions.SetStaticObjectField = [](JNIEnv* env, jclass klass, jfieldID field, jobject value) -> void {
    auto clsInstance = JniTranslate<jclass, ClassInstance*>{}(klass);
    auto fieldOffset = JniTranslate<jfieldID, JField*>{}(field)->offset();
    auto translatedValue = JniTranslate<jobject, Instance*>{}(value);

    clsInstance->target()->setStaticFieldValue(fieldOffset, Value::from<Instance*>(translatedValue));
  };

  functions.GetLongField = getField<int64_t>;
  functions.GetObjectField = [](JNIEnv* env, jobject object, jfieldID field) {
    auto objectInstance = JniTranslate<jobject, Instance*>{}(object);
    auto fieldOffset = JniTranslate<jfieldID, JField*>{}(field)->offset();

    Instance* ret = objectInstance->getFieldValue<Instance*>(fieldOffset);
    return JniTranslate<Instance*, jobject>{}(ret);
  };
}

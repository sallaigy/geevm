#include "JniImplementation.h"

#include "Instance.h"

#include <jni.h>

using namespace geevm;

JniImplementation::JniImplementation()
  : mEnv(), mFunctions()
{
  mFunctions.GetVersion = [](JNIEnv* env) {
    return JNI_VERSION_10;
  };

  mFunctions.GetObjectClass = [](JNIEnv* env, jobject obj) -> jclass {
    Instance* objectRef = JniTranslate<jobject, Instance*>{}(obj);
    return JniTranslate<ClassInstance*, jclass>{}(objectRef->getClass()->classInstance()->asClassInstance());
  };

  mEnv.functions = &mFunctions;
}

JNIEnv_* JniImplementation::getEnv()
{
  return &mEnv;
}

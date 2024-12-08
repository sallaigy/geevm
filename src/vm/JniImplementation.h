#ifndef GEEVM_JNIIMPLEMENTATION_H
#define GEEVM_JNIIMPLEMENTATION_H

#include <bit>
#include <jni.h>

namespace geevm
{

class Instance;
class ClassInstance;

template<class From, class To>
struct JniTranslateImpl
{
  To operator()(From from) const
  {
    return std::bit_cast<To>(from);
  }
};

template<class From, class To>
struct JniTranslate;

template<>
struct JniTranslate<jobject, Instance*> : JniTranslateImpl<jobject, Instance*>
{
};

template<>
struct JniTranslate<Instance*, jobject> : JniTranslateImpl<Instance*, jobject>
{
};

template<>
struct JniTranslate<jclass, ClassInstance*> : JniTranslateImpl<jclass, ClassInstance*>
{
};

template<>
struct JniTranslate<ClassInstance*, jclass> : JniTranslateImpl<ClassInstance*, jclass>
{
};

class JniImplementation
{
public:
  JniImplementation();

  JNIEnv_* getEnv();

private:
  JNIEnv_ mEnv;
  JNINativeInterface_ mFunctions;
};

} // namespace geevm

#endif // GEEVM_JNIIMPLEMENTATION_H

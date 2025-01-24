
#include <vm/Heap.h>
#include <vm/Instance.h>
#include <vm/JniImplementation.h>
#include <vm/VmUtils.h>

#include <algorithm>
#include <cassert>
#include <jni.h>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_lang_Class_registerNatives(JNIEnv* env, jclass klass)
{
}

JNIEXPORT jboolean JNICALL Java_java_lang_Class_desiredAssertionStatus0(JNIEnv* env, jclass klass)
{
  return JNI_FALSE;
}

JNIEXPORT jclass JNICALL Java_java_lang_Class_getPrimitiveClass(JNIEnv* env, jclass, jstring name)
{
  const char* buffer = env->GetStringUTFChars(name, nullptr);

  static std::unordered_map<std::string_view, std::string_view> classNames = {
      {"float", "java/lang/Float"}, {"double", "java/lang/Double"}, {"int", "java/lang/Integer"},    {"byte", "java/lang/Byte"},
      {"short", "java/lang/Short"}, {"long", "java/lang/Long"},     {"char", "java/lang/Character"}, {"boolean", "java/lang/Boolean"}};

  assert(classNames.contains(buffer));

  auto className = classNames.at(buffer);
  auto loaded = env->FindClass(className.data());

  env->ReleaseStringUTFChars(name, buffer);

  return loaded;
}

JNIEXPORT jboolean JNICALL Java_java_lang_Class_isPrimitive(JNIEnv* env, jclass klass)
{
  auto classObject = JniTranslate<jclass, GcRootRef<ClassInstance>>{}(klass);
  const types::JString& className = classObject->target()->className();

  static std::unordered_set<types::JString> klassNames = {
      u"java/lang/Boolean", u"java/Lang/Float", u"java/lang/Double",  u"java/lang/Byte",
      u"java/lang/Char",    u"java/lang/Short", u"java/lang/Integer", u"java/lang/Long",
  };

  return klassNames.contains(className) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jobject JNICALL Java_java_lang_Class_getName0(JNIEnv* env, jclass klass)
{
  GcRootRef<ClassInstance> cls = JniTranslate<jclass, GcRootRef<ClassInstance>>{}(klass);
  assert(cls != nullptr);

  auto name = cls->target()->className();
  std::ranges::replace(name, u'/', u'.');

  GcRootRef<> str = jni::threadFromJniEnv(env).heap().intern(name);
  return JniTranslate<GcRootRef<Instance>, jobject>{}(str);
}

JNIEXPORT jclass JNICALL Java_java_lang_Class_forName0(JNIEnv* env, jclass klass, jstring name, jboolean initialize, jobject classLoader, jclass caller)
{
  assert(classLoader == nullptr && "TODO: Support non-boostrap classloader");
  auto nameStr = env->GetStringUTFChars(name, nullptr);

  auto loaded = env->FindClass(nameStr);

  env->ReleaseStringUTFChars(name, nameStr);
  return loaded;
}

JNIEXPORT jobject JNICALL Java_java_lang_Class_initClassName(JNIEnv* env, jclass klass)
{
  GcRootRef<ClassInstance> clsInstance = JniTranslate<jclass, GcRootRef<ClassInstance>>{}(klass);

  auto name = clsInstance->target()->className();
  std::ranges::replace(name, u'/', u'.');

  GcRootRef<> str = jni::threadFromJniEnv(env).heap().intern(name);
  return JniTranslate<GcRootRef<Instance>, jobject>{}(str);
}
}

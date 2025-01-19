
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

JNIEXPORT jclass JNICALL Java_java_lang_Class_getPrimitiveClass(JNIEnv* env, jclass klass, jstring name)
{
  auto stringObject = geevm::JniTranslate<jobject, GcRootRef<Instance>>{}(name);
  types::JString buffer = utils::getStringValue(stringObject.get());

  static std::unordered_map<types::JStringRef, types::JStringRef> classNames = {
      {u"float", u"java/lang/Float"}, {u"double", u"java/lang/Double"}, {u"int", u"java/lang/Integer"},   {u"byte", u"java/lang/Byte"},
      {u"short", u"java/lang/Short"}, {u"long", u"java/lang/Long"},     {u"char", u"java/lang/Character"}};

  assert(classNames.contains(buffer));

  auto className = types::convertJString(types::JString{classNames.at(buffer)});
  auto loaded = env->FindClass(className.c_str());

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

  Instance* str = jni::threadFromJniEnv(env).heap().intern(name);
  GcRootRef<> strRef = jni::threadFromJniEnv(env).heap().gc().pin(str).release();

  return JniTranslate<GcRootRef<Instance>, jobject>{}(strRef);
}

JNIEXPORT jclass JNICALL Java_java_lang_Class_forName0(JNIEnv* env, jclass klass, jstring name, jboolean initialize, jobject classLoader, jclass caller)
{
  GcRootRef<Instance> nameObject = JniTranslate<jobject, GcRootRef<Instance>>{}(name);

  assert(classLoader == nullptr && "TODO: Support non-boostrap classloader");
  types::JString nameStr = utils::getStringValue(nameObject.get());

  std::string nameStrUtf8 = types::convertJString(nameStr);
  auto loaded = env->FindClass(nameStrUtf8.data());

  return loaded;
}

JNIEXPORT jobject JNICALL Java_java_lang_Class_initClassName(JNIEnv* env, jclass klass)
{
  GcRootRef<ClassInstance> clsInstance = JniTranslate<jclass, GcRootRef<ClassInstance>>{}(klass);

  auto name = clsInstance->target()->className();
  std::ranges::replace(name, u'/', u'.');

  Instance* str = jni::threadFromJniEnv(env).heap().intern(name);
  GcRootRef<Instance> strRef = jni::threadFromJniEnv(env).heap().gc().pin(str).release();

  return JniTranslate<GcRootRef<Instance>, jobject>{}(strRef);
}
}

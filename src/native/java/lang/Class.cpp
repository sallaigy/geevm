
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
  auto stringObject = geevm::JniTranslate<jobject, Instance*>{}(name);
  types::JString buffer = utils::getStringValue(stringObject);

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
  auto classObject = JniTranslate<jclass, ClassInstance*>{}(klass);
  const types::JString& className = classObject->target()->className();

  static std::unordered_set<types::JString> klassNames = {
      u"java/lang/Boolean", u"java/Lang/Float", u"java/lang/Double",  u"java/lang/Byte",
      u"java/lang/Char",    u"java/lang/Short", u"java/lang/Integer", u"java/lang/Long",
  };

  return klassNames.contains(className) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jobject JNICALL Java_java_lang_Class_getName0(JNIEnv* env, jclass klass)
{
  ClassInstance* cls = JniTranslate<jclass, ClassInstance*>{}(klass);
  assert(cls != nullptr);

  auto name = cls->target()->className();
  std::ranges::replace(name, u'/', u'.');

  Instance* str = jni::threadFromJniEnv(env).heap().intern(name);

  return JniTranslate<Instance*, jobject>{}(str);
}

JNIEXPORT jclass JNICALL Java_java_lang_Class_forName0(JNIEnv* env, jclass klass, jstring name, jboolean initialize, jobject classLoader)
{
  Instance* nameObject = JniTranslate<jobject, Instance*>{}(name);

  assert(classLoader == nullptr && "TODO: Support non-boostrap classloader");
  types::JString nameStr = utils::getStringValue(nameObject);

  std::string nameStrUtf8 = types::convertJString(nameStr);
  auto loaded = env->FindClass(nameStrUtf8.data());

  return loaded;
}

JNIEXPORT jarray JNICALL Java_java_lang_Class_getDeclaredFields0(JNIEnv* env, jclass klass, jboolean isPublicOnly)
{
  jclass fieldCls = env->FindClass("java/lang/reflect/Field");
  if (fieldCls == nullptr) {
    return nullptr;
  }

  ClassInstance* fieldClsInstance = JniTranslate<jclass, ClassInstance*>{}(fieldCls);
  ClassInstance* clsInstance = JniTranslate<jclass, ClassInstance*>{}(klass);
  JavaThread& thread = jni::threadFromJniEnv(env);

  std::vector<Instance*> fields;
  for (auto& [nameAndDescriptor, field] : clsInstance->target()->fields()) {
    if (!isPublicOnly || field->isPublic()) {
      Instance* fieldInstance = thread.heap().allocate(fieldClsInstance->target()->asInstanceClass());
      fieldInstance->setFieldValue(u"name", u"Ljava/lang/String;", thread.heap().intern(field->name()));
      fieldInstance->setFieldValue(u"modifiers", u"I", static_cast<int32_t>(field->accessFlags()));

      fields.push_back(fieldInstance);
    }
  }

  ArrayInstance* fieldsArray = thread.heap().allocateArray((*thread.resolveClass(u"[Ljava/lang/reflect/Field;"))->asArrayClass(), fields.size());
  for (int i = 0; i < fields.size(); i++) {
    fieldsArray->setArrayElement(i, fields.at(i));
  }

  return JniTranslate<ArrayInstance*, jarray>{}(fieldsArray);
}

JNIEXPORT jobject JNICALL Java_java_lang_Class_initClassName(JNIEnv* env, jclass klass)
{
  ClassInstance* clsInstance = JniTranslate<jclass, ClassInstance*>{}(klass);

  auto name = clsInstance->target()->className();
  std::ranges::replace(name, u'/', u'.');

  Instance* str = jni::threadFromJniEnv(env).heap().intern(name);

  return JniTranslate<Instance*, jobject>{}(str);
}
}

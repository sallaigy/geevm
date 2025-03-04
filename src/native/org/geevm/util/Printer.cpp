#include "common/Encoding.h"
#include "vm/Class.h"
#include "vm/Instance.h"
#include "vm/JniImplementation.h"
#include "vm/Thread.h"
#include "vm/VmUtils.h"

#include <iostream>

using namespace geevm;

extern "C"
{
JNIEXPORT void JNICALL Java_org_geevm_util_Printer_println__I(JNIEnv*, jclass, jint value)
{
  std::cout << value << std::endl;
}

JNIEXPORT void JNICALL Java_org_geevm_util_Printer_println__F(JNIEnv*, jclass, jfloat value)
{
  std::cout << value << std::endl;
}

JNIEXPORT void JNICALL Java_org_geevm_util_Printer_println__J(JNIEnv*, jclass, jlong value)
{
  std::cout << value << std::endl;
}

JNIEXPORT void JNICALL Java_org_geevm_util_Printer_println__D(JNIEnv*, jclass, jdouble value)
{
  std::cout << value << std::endl;
}

JNIEXPORT void JNICALL Java_org_geevm_util_Printer_println__C(JNIEnv*, jclass, jchar value)
{
  auto charValue = static_cast<char16_t>(value);
  std::cout << utf16ToUtf8(types::JString{charValue}) << std::endl;
}

JNIEXPORT void JNICALL Java_org_geevm_util_Printer_println__Z(JNIEnv*, jclass, jboolean value)
{
  std::cout << (value == 0 ? "false" : "true") << std::endl;
}

JNIEXPORT void JNICALL Java_org_geevm_util_Printer_println__Ljava_lang_String_2(JNIEnv*, jclass, jstring value)
{
  auto ref = JniTranslate<jstring, GcRootRef<JavaString>>{}(value);
  if (ref->getClass()->className() == u"java/lang/String") {
    types::JString out = utils::getStringValue(ref.get());
    std::cout << utf16ToUtf8(out) << std::endl;
  } else {
    std::cout << ref.get() << std::endl;
  }
}
}

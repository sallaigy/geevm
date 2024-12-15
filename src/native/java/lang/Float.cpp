#include <jni.h>

#include "vm/Instance.h"

#include <vm/JniImplementation.h>

using namespace geevm;

extern "C"
{

JNIEXPORT jint JNICALL Java_java_lang_Float_floatToRawIntBits(JNIEnv* env, jclass klass, jfloat value)
{
  return std::bit_cast<int32_t>(value);
}
}

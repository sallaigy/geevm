#include <jni.h>

#include "vm/Instance.h"

#include <vm/JniImplementation.h>

using namespace geevm;

extern "C"
{

JNIEXPORT jlong JNICALL Java_java_lang_Double_doubleToRawLongBits(JNIEnv*, jclass, jdouble value)
{
  return std::bit_cast<int64_t>(value);
}

JNIEXPORT jdouble JNICALL Java_java_lang_Double_longBitsToDouble(JNIEnv*, jclass, jlong value)
{
  return std::bit_cast<double>(value);
}
}

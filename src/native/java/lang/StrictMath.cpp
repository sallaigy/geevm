#include "vm/JniImplementation.h"

#include <cmath>

using namespace geevm;

extern "C"
{

JNIEXPORT jdouble JNICALL Java_java_lang_StrictMath_log(JNIEnv* env, jclass klass, jdouble value)
{
  // FIXME: use Java semantics (fdlibm)
  return std::log(value);
}
}

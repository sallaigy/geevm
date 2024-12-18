#include <jni.h>

#include "vm/Instance.h"

using namespace geevm;

extern "C"
{

JNIEXPORT jboolean JNICALL Java_jdk_internal_misc_CDS_isDumpingClassList0(JNIEnv*, jclass)
{
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_jdk_internal_misc_CDS_isDumpingArchive0(JNIEnv*, jclass)
{
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_jdk_internal_misc_CDS_isSharingEnabled0(JNIEnv*, jclass)
{
  return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_jdk_internal_misc_CDS_initializeFromArchive(JNIEnv*, jclass, jclass)
{
  // Ignore for now.
}

JNIEXPORT jlong JNICALL Java_jdk_internal_misc_CDS_getRandomSeedForDumping(JNIEnv*, jclass)
{
  return 0;
}
}

#include "vm/JniImplementation.h"

#include <fcntl.h>

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_io_FileDescriptor_initIDs(JNIEnv*, jclass)
{
  // No-op
}

JNIEXPORT jlong JNICALL Java_java_io_FileDescriptor_getHandle(JNIEnv*, jobject)
{
  // This is only relevant for Windows
  return -1;
}

JNIEXPORT jboolean JNICALL Java_java_io_FileDescriptor_getAppend(JNIEnv* env, jclass klass, jint fd)
{
  if (fcntl(fd, F_GETFD) & O_APPEND) {
    return JNI_TRUE;
  }
  return JNI_FALSE;
}
}

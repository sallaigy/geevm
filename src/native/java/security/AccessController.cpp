#include <jni.h>

extern "C"
{

JNIEXPORT jobject JNICALL Java_java_security_AccessController_getStackAccessControlContext(JNIEnv*, jclass)
{
  return nullptr;
}

}

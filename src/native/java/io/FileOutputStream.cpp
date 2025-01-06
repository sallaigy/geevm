#include "vm/Instance.h"
#include "vm/JniImplementation.h"
#include "vm/Thread.h"

#include <vector>

using namespace geevm;

extern "C"
{

JNIEXPORT void JNICALL Java_java_io_FileOutputStream_initIDs(JNIEnv*, jobject)
{
  // No-op
}

JNIEXPORT void JNICALL Java_java_io_FileOutputStream_writeBytes(JNIEnv* env, jobject fos, jbyteArray bytes, jint offset, jint length, jboolean append)
{
  JavaThread& thread = jni::threadFromJniEnv(env);
  auto klass = env->GetObjectClass(fos);
  auto descriptorField = env->GetFieldID(klass, "fd", "Ljava/io/FileDescriptor;");
  if (descriptorField == nullptr) {
    return;
  }

  auto descriptor = env->GetObjectField(fos, descriptorField);
  auto descriptorClass = env->GetObjectClass(descriptor);

  auto fdField = env->GetFieldID(descriptorClass, "fd", "I");
  if (fdField == nullptr) {
    return;
  }

  auto fd = env->GetIntField(descriptor, fdField);

  auto array = JniTranslate<jbyteArray, GcRootRef<JavaArray<int8_t>>>{}(bytes);
  std::vector<jbyte> buffer;
  for (int32_t i = 0; i < length; i++) {
    auto res = array->getArrayElement(i);
    if (!res) {
      thread.throwException(res.error().exception());
      return;
    } else {
      buffer.push_back(*res);
    }
  }

  FILE* fp = fdopen(fd, "w");
  if (fp == nullptr) {
    thread.throwException(u"java/io/IOException", u"failed to open file");
    return;
  }

  fwrite(buffer.data() + offset, sizeof(int8_t), length, fp);
  fflush(fp);

  // Do not attempt to close stdout and stderr
  if (fd > STDERR_FILENO) {
    fclose(fp);
  }
}
}

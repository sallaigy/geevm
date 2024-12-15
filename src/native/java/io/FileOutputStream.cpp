#include "vm/Instance.h"
#include "vm/JniImplementation.h"

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

  ArrayInstance* array = JniTranslate<jbyteArray, ArrayInstance*>{}(bytes);
  std::vector<jbyte> buffer;
  for (int32_t i = 0; i < length; i++) {
    buffer.push_back(array->getArrayElement(i)->get<int8_t>());
  }

  FILE* fp = fdopen(fd, "w");
  fwrite(reinterpret_cast<char*>(buffer.data() + offset), sizeof(int8_t), length, fp);
}
}

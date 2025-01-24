#include "vm/JniImplementation.h"
#include "vm/VmUtils.h"

#include <csignal>
#include <jni.h>
#include <unordered_map>

using namespace geevm;

extern "C"
{

JNIEXPORT jint JNICALL Java_jdk_internal_misc_Signal_findSignal0(JNIEnv* env, jclass, jstring signal)
{
  static std::unordered_map<std::string_view, int32_t> signalMap{{"HUP", SIGHUP}, {"INT", SIGINT}, {"TERM", SIGTERM}};

  auto nameStr = env->GetStringUTFChars(signal, nullptr);

  if (auto it = signalMap.find(nameStr); it != signalMap.end()) {
    env->ReleaseStringUTFChars(signal, nameStr);
    return it->second;
  }

  env->ReleaseStringUTFChars(signal, nameStr);

  return -1;
}

JNIEXPORT jlong JNICALL Java_jdk_internal_misc_Signal_handle0(JNIEnv*, jclass, jclass, jint, jlong)
{
  return 0;
}
}

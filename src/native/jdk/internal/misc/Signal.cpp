#include "vm/JniImplementation.h"
#include "vm/VmUtils.h"

#include <csignal>
#include <jni.h>
#include <unordered_map>

using namespace geevm;

extern "C"
{

JNIEXPORT jint JNICALL Java_jdk_internal_misc_Signal_findSignal0(JNIEnv*, jclass, jstring signal)
{
  static std::unordered_map<types::JStringRef, int32_t> signalMap{{u"HUP", SIGHUP}};

  auto nameStr = utils::getStringValue(JniTranslate<jobject, GcRootRef<Instance>>{}(signal).get());

  if (auto it = signalMap.find(nameStr); it != signalMap.end()) {
    return it->second;
  }

  return -1;
}

JNIEXPORT jlong JNICALL Java_jdk_internal_misc_Signal_handle0(JNIEnv*, jclass, jclass, jint, jlong)
{
  return 0;
}
}

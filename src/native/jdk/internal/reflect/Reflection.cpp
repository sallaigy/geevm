#include "vm/Heap.h"
#include "vm/JniImplementation.h"
#include "vm/Thread.h"

#include <filesystem>

using namespace geevm;

extern "C"
{
JNIEXPORT jclass JNICALL Java_jdk_internal_reflect_Reflection_getCallerClass(JNIEnv* env, jclass)
{
  JavaThread& thread = jni::threadFromJniEnv(env);
  const CallFrame& frame = thread.currentFrame();

  // Returns the class of the caller of the method calling this method ignoring frames associated with java.lang.reflect.Method.invoke() and its implementation.
  CallFrame* previous = frame.previous()->previous();

  return jni::translate(previous->currentClass()->classInstance());
}
}

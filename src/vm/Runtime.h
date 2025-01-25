#ifndef GEEVM_RUNTIME_H
#define GEEVM_RUNTIME_H

#include "GarbageCollector.h"
#include "class_file/ConstantPool.h"

#include <common/JvmError.h>
#include <unordered_map>

namespace geevm
{

class MethodRef;
class JMethod;
class FieldRef;
class JField;
class Instance;
class JClass;
class JavaHeap;
class BootstrapClassLoader;

class RuntimeConstantPool
{
public:
  RuntimeConstantPool(const ConstantPool& constantPool, JavaHeap& heap, BootstrapClassLoader& bootstrapClassLoader)
    : mConstantPool(constantPool), mHeap(heap), mBootstrapClassLoader(bootstrapClassLoader)
  {
  }

  GcRootRef<Instance> getString(types::u2 index);

  JMethod* getMethodRef(types::u2 index);
  JField* getFieldRef(types::u2 index);
  JvmExpected<JClass*> getClass(types::u2 index);

  types::JStringRef getUtf8(types::u2 index);

private:
  std::unordered_map<types::u2, JMethod*> mMethodRefs;
  std::unordered_map<types::u2, JField*> mFieldRefs;
  std::unordered_map<types::u2, GcRootRef<>> mStrings;
  std::unordered_map<types::u2, JClass*> mClasses;

  const ConstantPool& mConstantPool;
  JavaHeap& mHeap;
  BootstrapClassLoader& mBootstrapClassLoader;
};

} // namespace geevm

#endif // GEEVM_RUNTIME_H

#include "vm/GarbageCollector.h"

#include "common/Debug.h"
#include "common/JvmError.h"
#include "common/Memory.h"
#include "vm/Class.h"
#include "vm/Vm.h"

#include <cstdlib>
#include <cstring>

using namespace geevm;

GarbageCollector::GarbageCollector(Vm& vm)
  : mVm(vm), mHeapSize(vm.settings().maxHeapSize), mRunAfterEveryAllocation(vm.settings().runGcAfterEveryAllocation)
{
  mFromRegion = static_cast<char*>(::operator new(mHeapSize / 2));
  mToRegion = static_cast<char*>(::operator new(mHeapSize / 2));
  std::memset(mFromRegion, 0, mHeapSize / 2);
  mBumpPtr = mFromRegion;

  ASAN_POISON_MEMORY_REGION(mToRegion, mHeapSize / 2);
}

void GarbageCollector::lockGC()
{
  mIsGcRunning = true;
}

void GarbageCollector::unlockGC()
{
  mIsGcRunning = false;
}

void* GarbageCollector::allocate(size_t size)
{
  const char* end = mFromRegion + mHeapSize / 2;
  if (mBumpPtr + size > end) {
    // The 'from' region is full, let's do GC
    this->performGarbageCollection();
    const char* end2 = mFromRegion + mHeapSize / 2;
    if (mBumpPtr + size > end2) {
      // TODO: Throw OutOfMemoryException
      geevm_panic("out of heap memory");
    }
  } else if (mRunAfterEveryAllocation) {
    this->performGarbageCollection();
  }

  return this->allocateUnchecked(size);
}

void* GarbageCollector::allocateUnchecked(size_t size)
{
  void* current = mBumpPtr;

  size_t adjustedSize = alignTo(size, alignof(std::max_align_t));
  mBumpPtr += adjustedSize;

  return current;
}

Instance* GarbageCollector::copyObject(Instance* instance, std::unordered_map<Instance*, Instance*>& map)
{
  if (instance == nullptr) {
    // No need to copy null
    return nullptr;
  }

  if (auto it = map.find(instance); it != map.end()) {
    // This has already been copied
    return it->second;
  }

  auto klass = instance->getClass();
  std::size_t objectSize = 0;
  if (auto arrayClass = klass->asArrayClass(); arrayClass) {
    objectSize = arrayClass->allocationSize(instance->asArrayInstance()->length());
  } else {
    objectSize = klass->asInstanceClass()->allocationSize();
  }

  void* mem = this->allocateUnchecked(objectSize);
  std::memcpy(mem, instance, objectSize);
  auto* copy = static_cast<Instance*>(mem);
  map.insert({instance, copy});

  return copy;
}

void GarbageCollector::performGarbageCollection()
{
  if (mIsGcRunning) {
    return;
  }
  this->lockGC();

  ASAN_UNPOISON_MEMORY_REGION(mToRegion, mHeapSize / 2);
  std::swap(mFromRegion, mToRegion);
  mBumpPtr = mFromRegion;

  std::unordered_map<Instance*, Instance*> map;
  char* scanPtr = mFromRegion;

  // Update manually pinned roots
  for (auto& root : mRootList) {
    Instance* copy = this->copyObject(root, map);
    root = copy;
  }

  for (const auto& [_, klass] : mVm.bootstrapClassLoader().loadedClasses()) {
    // Copy and update the class instance
    // The class instance can be null if GC was triggered during class initialization
    for (auto& [_, field] : klass->fields()) {
      if (field->isStatic() && field->fieldType().isReferenceOrArray()) {
        auto* instance = klass->getStaticFieldValue<Instance*>(field->offset());
        auto* copy = this->copyObject(instance, map);
        klass->setStaticFieldValue<Instance*>(field->offset(), copy);
      }
    }
  }

  // Update local variables and the stack in threads
  for (JavaThread* thread : mVm.threads()) {
    for (CallFrame& frame : thread->callStack()) {
      for (types::u2 i = 0; i < frame.locals().size(); i++) {
        if (auto object = frame.locals()[i].tryGet<Instance*>(); object) {
          Instance* copy = this->copyObject(*object, map);
          frame.storeValue(i, copy);
        }
      }

      for (types::u2 i = 0; i < frame.operandStack().size(); i++) {
        if (auto object = frame.operandStack()[i].tryGet<Instance*>(); object) {
          Instance* copy = this->copyObject(*object, map);
          frame.replaceStackValue(i, Value::from(copy));
        }
      }
    }
  }

  while (scanPtr < mBumpPtr) {
    auto* instance = reinterpret_cast<Instance*>(scanPtr);
    size_t objectSize = this->processReferences(instance, map);

    size_t adjustedSize = alignTo(objectSize, alignof(std::max_align_t));
    scanPtr += adjustedSize;
  }

  // Clear up the previous region
  std::memset(mToRegion, 0, mHeapSize / 2);
  ASAN_POISON_MEMORY_REGION(mToRegion, mHeapSize / 2);

  this->unlockGC();
}

size_t GarbageCollector::processReferences(Instance* instance, std::unordered_map<Instance*, Instance*>& map)
{
  auto klass = instance->getClass();
  std::size_t objectSize = 0;
  if (auto arrayClass = klass->asArrayClass(); arrayClass) {
    objectSize = arrayClass->allocationSize(instance->asArrayInstance()->length());
  } else {
    objectSize = klass->asInstanceClass()->allocationSize();
  }

  if (auto instanceClass = klass->asInstanceClass(); instanceClass) {
    for (auto& [_, field] : klass->fields()) {
      if (!field->isStatic() && field->fieldType().isReferenceOrArray()) {
        auto* fieldValue = instance->getFieldValue<Instance*>(field->offset());
        Instance* copiedField = this->copyObject(fieldValue, map);
        instance->setFieldValue<Instance*>(field->offset(), copiedField);
      }
    }
  } else if (auto arrayClass = klass->asArrayClass(); arrayClass) {
    auto elementType = arrayClass->fieldType().asArrayType()->getElementType();

    if (arrayClass->fieldType().asArrayType()->getElementType().isReferenceOrArray()) {
      JavaArray<Instance*>* arrayOfObjects = instance->asArray<Instance*>();
      for (int32_t i = 0; i < arrayOfObjects->length(); i++) {
        Instance* elem = *arrayOfObjects->getArrayElement(i);
        Instance* copyOfElem = this->copyObject(elem, map);

        arrayOfObjects->setArrayElement(i, copyOfElem);
      }
    }
  } else {
    GEEVM_UNREACHBLE("A class must be either an instance class or an array")
  }

  return objectSize;
}

GarbageCollector::~GarbageCollector()
{
  ::operator delete(mFromRegion);
  ::operator delete(mToRegion);
}

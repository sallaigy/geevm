#include "vm/GarbageCollector.h"

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
  char* end = mFromRegion + mHeapSize / 2;
  if (mBumpPtr + size > end) {
    // The 'from' region is full, let's do GC
    this->performGarbageCollection();
    char* end2 = mFromRegion + mHeapSize / 2;
    if (mBumpPtr + size > end2) {
      // FIXME: This should be JVMExpected
      return nullptr;
    }
  } else if (mRunAfterEveryAllocation) {
    this->performGarbageCollection();
  }

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

  void* mem = this->allocate(objectSize);
  std::memcpy(mem, instance, objectSize);
  Instance* copy = reinterpret_cast<Instance*>(mem);
  map.insert({instance, copy});

  if (auto instanceClass = klass->asInstanceClass(); instanceClass) {
    for (auto& [_, field] : klass->fields()) {
      if (!field->isStatic() && field->fieldType().isReferenceOrArray()) {
        auto* fieldValue = instance->getFieldValue<Instance*>(field->offset());
        Instance* copiedField = this->copyObject(fieldValue, map);
        copy->setFieldValue<Instance*>(field->offset(), copiedField);
      }
    }
  } else if (auto arrayClass = klass->asArrayClass(); arrayClass) {
    auto elementType = arrayClass->fieldType().asArrayType()->getElementType();

    map.insert({instance, copy});
    if (arrayClass->fieldType().asArrayType()->getElementType().isReferenceOrArray()) {
      JavaArray<Instance*>* arrayOfObjects = instance->asArray<Instance*>();
      for (int32_t i = 0; i < arrayOfObjects->length(); i++) {
        Instance* elem = *arrayOfObjects->getArrayElement(i);
        Instance* copyOfElem = this->copyObject(elem, map);

        if (elem != nullptr) {
          assert((elem->getClass() == nullptr && copyOfElem->getClass() == nullptr) || elem->getClass()->className() == copyOfElem->getClass()->className());
        }
        copy->asArray<Instance*>()->setArrayElement(i, copyOfElem);
      }
    }
  } else {
    assert(false);
  }

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

  // Update manually pinned roots
  for (auto it = mRootList.begin(); it != mRootList.end(); ++it) {
    Instance* copy = this->copyObject(*it, map);
    *it = copy;
  }

  for (auto& [_, klass] : mVm.bootstrapClassLoader().loadedClasses()) {
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

  // Clear up the previous region
  std::memset(mToRegion, 0, mHeapSize / 2);
  ASAN_POISON_MEMORY_REGION(mToRegion, mHeapSize / 2);

  this->unlockGC();
}

GarbageCollector::~GarbageCollector()
{
  ::operator delete(mFromRegion);
  ::operator delete(mToRegion);
}

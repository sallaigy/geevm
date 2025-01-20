#include "BaseTest.h"

#include "vm/GarbageCollector.h"
#include "vm/Vm.h"

#include <gmock/gmock.h>

using namespace geevm;

static const std::string HelloWorldClass = "class_file/org/geevm/tests/classfile/HelloWorld.class";

class GarbageCollectorTest : public geevm::testing::BaseTest
{
public:
  explicit GarbageCollectorTest()
    : mThread(mVm), mHelloWorldClass(ClassFile::fromFile(getResource(HelloWorldClass)))
  {
  }

  GarbageCollector& gc()
  {
    return mVm.heap().gc();
  }

protected:
  Vm mVm{VmSettings{}};
  JavaThread mThread;
  InstanceClass mHelloWorldClass;
  Instance* mHelloWorldObject;
};

TEST_F(GarbageCollectorTest, allocate_and_pin_object)
{
  // Prevent GC from running during test setup
  gc().lockGC();
  auto object = mVm.heap().allocate(&mHelloWorldClass);
  gc().unlockGC();

  ASSERT_EQ(object->getClass()->className(), u"org/geevm/tests/classfile/HelloWorld");

  // Pinning makes an object into a "GC root"
  auto pinned = gc().pin(object);

  ASSERT_EQ(pinned, object);
  gc().performGarbageCollection();

  // The underlying object got relocated, so the pointers should not be equal anymore
  ASSERT_NE(pinned, object);

  // Because the object was pinned, this access is still valid
  ASSERT_EQ(pinned->getClass()->className(), u"org/geevm/tests/classfile/HelloWorld");
}

TEST_F(GarbageCollectorTest, allocate_and_pin_array)
{
  // Prevent GC from running during test setup
  gc().lockGC();

  types::JString className = u"[Lorg/geevm/tests/classfile/HelloWorld;";
  ArrayClass arrayClass{className, FieldType::parse(className).value()};
  auto array = mVm.heap().allocateArray<Instance*>(&arrayClass, 10);

  std::vector<Instance*> createdObjects;
  for (size_t i = 0; i < 10; i++) {
    auto object = mVm.heap().allocate(&mHelloWorldClass);
    array->setArrayElement(i, object);
    createdObjects.push_back(object);
  }

  gc().unlockGC();

  auto pinned = gc().pin(array);
  ASSERT_EQ(pinned, array);

  gc().performGarbageCollection();

  ASSERT_NE(pinned, array);
  ASSERT_EQ(pinned->getClass(), &arrayClass);

  // All memory accesses should still be valid
  for (size_t i = 0; i < 10; i++) {
    Instance* object = pinned->getArrayElement(i).value();
    // The object got relocated, so it should be a different pointer
    ASSERT_NE(object, createdObjects[i]);
    // But the memory access should be valid
    ASSERT_EQ(object->getClass(), &mHelloWorldClass);
  }
}

TEST_F(GarbageCollectorTest, compare_root_ref)
{
  gc().lockGC();
  auto object = mVm.heap().allocate(&mHelloWorldClass);
  gc().unlockGC();

  GcRootRef<> ref1 = gc().pin(object).release();
  GcRootRef<> ref2 = gc().pin(object).release();

  ASSERT_TRUE(ref1 == ref2);
  ASSERT_TRUE(ref1 == object);
}

TEST_F(GarbageCollectorTest, scoped_root_ref_move)
{
  auto object = mVm.heap().allocate(&mHelloWorldClass);

  ScopedGcRootRef<> ref1 = gc().pin(object);
  ASSERT_TRUE(ref1 == object);

  ScopedGcRootRef<> moveConstructed = std::move(ref1);
  ASSERT_TRUE(moveConstructed == object);

  auto anotherObject = mVm.heap().allocate(&mHelloWorldClass);

  ScopedGcRootRef<> moveAssigned = gc().pin(anotherObject);
  moveAssigned = std::move(moveConstructed);

  ASSERT_TRUE(moveAssigned == object);
}

TEST_F(GarbageCollectorTest, compare_root_ref_to_null)
{
  auto object = mVm.heap().allocate(&mHelloWorldClass);

  auto pinned = gc().pin(object).release();

  ASSERT_FALSE(pinned == nullptr);

  GcRootRef<> nullRef = nullptr;
  ASSERT_TRUE(nullRef == nullptr);

  ASSERT_FALSE(pinned == nullRef);
}

TEST_F(GarbageCollectorTest, compare_scoped_root_ref_to_null)
{
  gc().lockGC();
  auto object = mVm.heap().allocate(&mHelloWorldClass);
  gc().unlockGC();

  auto pinned = gc().pin(object);

  ASSERT_FALSE(pinned == nullptr);
}

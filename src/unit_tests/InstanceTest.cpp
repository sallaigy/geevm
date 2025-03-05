#include "class_file/ClassFile.h"
#include "unit_tests/BaseTest.h"
#include "vm/Class.h"
#include "vm/Vm.h"

using namespace geevm;

class InstanceTest : public geevm::testing::BaseTest
{
public:
  InstanceTest()
    : mVm(VmSettings{})
  {
    mVm.bootstrapClassLoader().classPath().addDirectory(std::string{Jdk17Path} + "/modules/java.base");
  }

  JClass* loadClass(const types::JString& name)
  {
    return *mVm.bootstrapClassLoader().loadClass(name);
  }

protected:
  Vm mVm;
};

TEST_F(InstanceTest, string_instance_layout)
{
  auto stringClass = loadClass(u"java/lang/String");

  stringClass->prepare(mVm.bootstrapClassLoader(), mVm.heap());

  auto valueField = stringClass->lookupField(u"value", u"[B");
  auto coderField = stringClass->lookupField(u"coder", u"B");
  auto hashField = stringClass->lookupField(u"hash", u"I");
  auto hashIsZeroField = stringClass->lookupField(u"hashIsZero", u"Z");

  ASSERT_EQ(offsetof(JavaString, mValue), (*valueField)->offset());
  ASSERT_EQ(offsetof(JavaString, mCoder), (*coderField)->offset());
  ASSERT_EQ(offsetof(JavaString, mHash), (*hashField)->offset());
  ASSERT_EQ(offsetof(JavaString, mHashIsZero), (*hashIsZeroField)->offset());
}

TEST_F(InstanceTest, throwable_instance_layout)
{
  auto throwableClass = loadClass(u"java/lang/Throwable");
  throwableClass->prepare(mVm.bootstrapClassLoader(), mVm.heap());

  auto backtraceField = throwableClass->lookupField(u"backtrace", u"Ljava/lang/Object;");
  auto detailMessage = throwableClass->lookupField(u"detailMessage", u"Ljava/lang/String;");
  auto cause = throwableClass->lookupField(u"cause", u"Ljava/lang/Throwable;");
  auto stackTrace = throwableClass->lookupField(u"stackTrace", u"[Ljava/lang/StackTraceElement;");
  auto depth = throwableClass->lookupField(u"depth", u"I");
  auto suppressedExceptions = throwableClass->lookupField(u"suppressedExceptions", u"Ljava/util/List;");

  ASSERT_EQ(offsetof(JavaThrowable, mBacktrace), (*backtraceField)->offset());
  ASSERT_EQ(offsetof(JavaThrowable, mDetailMessage), (*detailMessage)->offset());
  ASSERT_EQ(offsetof(JavaThrowable, mCause), (*cause)->offset());
  ASSERT_EQ(offsetof(JavaThrowable, mStackTrace), (*stackTrace)->offset());
  ASSERT_EQ(offsetof(JavaThrowable, mDepth), (*depth)->offset());
  ASSERT_EQ(offsetof(JavaThrowable, mSuppressedExceptions), (*suppressedExceptions)->offset());
}

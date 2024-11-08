#include "Vm.h"
#include "vm/Frame.h"

#include <iostream>

using namespace geevm;

void Vm::initialize()
{
  this->registerNatives();

  this->requireClass(u"java/lang/Object");
  this->requireClass(u"java/lang/Class");
  this->requireClass(u"java/lang/String");
  this->requireClass(u"java/lang/Throwable");

  auto threadGroupCls = this->requireClass(u"java/lang/ThreadGroup");

  auto mainThreadGroup = heap().allocate(threadGroupCls->asInstanceClass());
  mainThreadGroup->setFieldValue(u"name", Value::Reference(mHeap.intern(u"main")));
  mainThreadGroup->setFieldValue(u"maxPriority", Value::Int(10));

  this->requireClass(u"java/lang/Thread");
  mMainThread.initialize(u"main", mainThreadGroup);

  auto systemCls = this->requireClass(u"java/lang/System");

  auto fisCls = this->requireClass(u"java/io/FileInputStream");
  auto fosCls = this->requireClass(u"java/io/FileOutputStream");
  auto printStreamCls = this->requireClass(u"java/io/PrintStream");
  auto fdCls = this->requireClass(u"java/io/FileDescriptor");

  Instance* outFd = fdCls->getStaticField(u"out").asReference();
  auto fos = mMainThread.newInstance(u"java/io/FileOutputStream");
  fos->setFieldValue(u"fd", Value::Reference(outFd));
  fos->setFieldValue(u"append", Value::Int(0));

  Instance* bufferedOut = mainThread().newInstance(u"java/io/BufferedOutputStream");
  auto bufferedOutCtor = bufferedOut->getClass()->getMethod(u"<init>", u"(Ljava/io/OutputStream;)V");

  mMainThread.executeCall(*bufferedOutCtor, {Value::Reference(bufferedOut), Value::Reference(fos)});

  Instance* out = mMainThread.newInstance(u"java/io/PrintStream");
  auto psCtor = out->getClass()->getMethod(u"<init>", u"(ZLjava/io/OutputStream;)V");

  // mMainThread.executeCall(*psCtor, {Value::Reference(out), Value::Reference(bufferedOut)});
}

JvmExpected<JClass*> Vm::resolveClass(const types::JString& name)
{
  return mBootstrapClassLoader.loadClass(name);
}

JClass* Vm::requireClass(const types::JString& name)
{
  auto klass = this->resolveClass(name);
  assert(klass.has_value() && "Required classes must always be resolvable!");

  (*klass)->initialize(mMainThread);

  return *klass;
}

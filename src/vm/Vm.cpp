#include "Vm.h"
#include "vm/Frame.h"

#include <iostream>

using namespace geevm;

void Vm::initialize()
{
  mHeap.gc().lockGC();

  this->setUpJavaLangClass();
  this->requireClass(u"java/lang/Object");
  JClass* javaLangString = this->requireClass(u"java/lang/String");
  javaLangString->setStaticFieldValue(u"COMPACT_STRINGS", u"Z", Value::from<int32_t>(0));

  mHeap.initialize(javaLangString->asInstanceClass(), this->requireClass(u"[B")->asArrayClass());

  this->requireClass(u"java/lang/Throwable");

  auto threadGroupCls = this->requireClass(u"java/lang/ThreadGroup");

  auto mainThreadGroup = heap().allocate<Instance>(threadGroupCls->asInstanceClass());
  mainThreadGroup->setFieldValue<Instance*>(u"name", u"Ljava/lang/String;", mHeap.intern(u"main").get());
  mainThreadGroup->setFieldValue<int32_t>(u"maxPriority", u"I", 10);

  this->requireClass(u"java/lang/Thread");
  mMainThread->initialize(u"main", mainThreadGroup);

  JClass* systemCls = this->requireClass(u"java/lang/System");

  if (!settings().noSystemInit) {
    auto initMethod = systemCls->getMethod(u"initPhase1", u"()V");
    mMainThread->executeCall(*initMethod, {});
  }

  mHeap.gc().unlockGC();
}

JvmExpected<JClass*> Vm::resolveClass(const types::JString& name)
{
  return mBootstrapClassLoader.loadClass(name);
}

JClass* Vm::requireClass(const types::JString& name)
{
  auto klass = this->resolveClass(name);
  assert(klass.has_value() && "Required classes must always be resolvable!");

  (*klass)->initialize(*mMainThread);

  return *klass;
}

void Vm::setUpJavaLangClass()
{
  auto klass = this->bootstrapClassLoader().loadUnpreparedClass(u"java/lang/Class");
  assert(klass.has_value());

  auto classClass = (*klass)->asInstanceClass();
  classClass->linkFields();

  classClass->prepare(mBootstrapClassLoader, mHeap);
}

#include "Vm.h"
#include "vm/Frame.h"

#include <bits/ranges_algo.h>
#include <class_file/Attributes.h>
#include <iostream>

using namespace geevm;

void Vm::initialize()
{
  this->registerNatives();

  this->resolveClass(u"java/lang/Object");
  this->resolveClass(u"java/lang/Class");
  this->resolveClass(u"java/lang/String");

  auto threadGroupCls = this->resolveClass(u"java/lang/ThreadGroup");
  auto mainThreadGroup = heap().allocate((*threadGroupCls)->asInstanceClass());
  mainThreadGroup->setFieldValue(u"name", Value::Reference(mHeap.intern(u"main")));
  mainThreadGroup->setFieldValue(u"maxPriority", Value::Int(10));

  auto threadCls = this->resolveClass(u"java/lang/Thread");

  mMainThread.initialize(u"main", mainThreadGroup);

  // auto threadCls = this->resolveClass(u"java/lang/Thread");
  // auto threadGroupCls = this->resolveClass(u"java/lang/ThreadGroup");
  //
  // mCurrentThread = std::make_unique<Instance>(*threadCls);
  // mCurrentThread->setFieldValue(u"priority", Value::Int(1));
  //
  // auto mainThreadGroup = this->newInstance((*threadGroupCls)->asInstanceClass());
  // auto threadGroupCtor = (*threadGroupCls)->getMethod(u"<init>", u"(Ljava/lang/String;)V");
  // // this->execute(threadGroupCtor->klass, threadGroupCtor->method, {Value::Reference(mainThreadGroup), Value::Reference(mInternedStrings.intern(u"main"))});
  //
  // auto threadCtor = (*threadCls)->getMethod(u"<init>", u"(Ljava/lang/ThreadGroup;Ljava/lang/String;)V");
  // this->execute(threadCtor->klass, threadCtor->method,
  //               {Value::Reference(mCurrentThread.get()), Value::Reference(mainThreadGroup), Value::Reference(mInternedStrings.intern(u"init"))});
  //
  // auto systemCls = this->resolveClass(u"java/lang/System");
  // auto initMethod = (*systemCls)->getMethod(u"initializeSystemClass", u"()V");
  //
  // (*systemCls)->storeStaticField(u"lineSeparator", Value::Reference(mInternedStrings.intern(u"\n")));
  //
  // auto fisCls = this->resolveClass(u"java/io/FileInputStream");
  // auto fosCls = this->resolveClass(u"java/io/FileOutputStream");
  // auto printStreamCls = this->resolveClass(u"java/io/PrintStream");
  //
  // auto fdCls = this->resolveClass(u"java/io/FileDescriptor");
  //
  // auto fdCtor = (*fdCls)->getMethod(u"<init>", u"(I)V");
  // auto outFd = this->newInstance((*fdCls)->asInstanceClass());
  //
  // this->execute((*fdCls)->asInstanceClass(), fdCtor->method, std::vector<Value>{Value::Reference(outFd), Value::Int(1)});
  //
  // auto fosCtor = (*fosCls)->getMethod(u"<init>", u"(I)V");
  // auto outFos = this->newInstance((*fosCls)->asInstanceClass());
  //
  // this->execute(fosCtor->klass, fosCtor->method, {Value::Reference(outFos), Value::Reference(outFd)});
  //
  // auto outPs = this->newInstance((*printStreamCls)->asInstanceClass());
  // auto outPsCtor = (*printStreamCls)->getMethod(u"<init>", u"(Ljava/io/OutputStream;Z)V");
  //
  // this->execute(outPsCtor->klass, outPsCtor->method, {Value::Reference(outPs), Value::Reference(outFos), Value::Int(1)});
  //
  // (*systemCls)->storeStaticField(u"out", Value::Reference(outPs));

  // this->execute((*systemCls)->asInstanceClass(), initMethod->method);
}

JvmExpected<JClass*> Vm::resolveClass(const types::JString& name)
{
  return mBootstrapClassLoader.loadClass(name);
}

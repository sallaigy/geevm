#include "vm/NativeMethods.h"

#include "VmUtils.h"
#include "vm/Thread.h"
#include "vm/Vm.h"

#include <algorithm>
#include <csignal>
#include <iostream>
#include <utility>

using namespace geevm;

void NativeMethodRegistry::registerNativeMethod(const ClassNameAndDescriptor& method, NativeMethodHandle handle)
{
  mNativeMethods[method] = std::move(handle);
}

std::optional<NativeMethodHandle> NativeMethodRegistry::get(const ClassNameAndDescriptor& method) const
{
  if (auto it = mNativeMethods.find(method); it != mNativeMethods.end()) {
    return it->second;
  }

  return std::nullopt;
}

std::optional<NativeMethodHandle> NativeMethodRegistry::get(JMethod* method) const
{
  return get(ClassNameAndDescriptor{method->getClass()->className(), method->name(), method->rawDescriptor()});
}

static std::optional<Value> noop(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> return_nullptr(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Object_hashCode(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Object_getClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Object_wait(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_System_initProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_System_arraycopy(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Class_getPrimitiveClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_desiredAssertionStatus0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_getName0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_forName0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_getDeclaredFields0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Float_floatToRawIntBits(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Double_doubleToRawIntBits(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Double_longBitsToDouble(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> sun_misc_Unsafe_arrayBaseOffset(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> sun_misc_Unsafe_objectFieldOffset(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> sun_misc_Unsafe_compareAndSwapObject(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> sun_reflect_Reflection_getCallerClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_security_AccessController_doPrivileged(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Thread_currentThread(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Thread_isAlive(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Thread_start0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Throwable_fillInStackTrace(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Throwable_getStackTraceDepth(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Throwable_getStackTraceElement(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_String_intern(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> geevm_test_print(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Value value = args[0];

  const FieldType& type = frame.currentMethod()->descriptor().parameters().at(0);

  if (auto primitiveType = type.asPrimitive(); primitiveType) {
    switch (*primitiveType) {
      case PrimitiveType::Byte: std::cout << value.asInt() << std::endl; break;
      case PrimitiveType::Char: {
        char16_t charValue;
        if (value.kind() == Value::Kind::Char) {
          charValue = value.asChar();
        } else {
          charValue = static_cast<char16_t>(value.asInt());
        }

        std::cout << types::convertJString(types::JString{charValue}) << std::endl;
        break;
      }
      case PrimitiveType::Double: std::cout << value.asDouble() << std::endl; break;
      case PrimitiveType::Float: std::cout << value.asFloat() << std::endl; break;
      case PrimitiveType::Int: std::cout << value.asInt() << std::endl; break;
      case PrimitiveType::Long: std::cout << value.asLong() << std::endl; break;
      case PrimitiveType::Short: std::cout << value.asInt() << std::endl; break;
      case PrimitiveType::Boolean: std::cout << (value.asInt() == 1 ? "true" : "false") << std::endl; break;
    }
  } else {
    Instance* ref = value.asReference();
    if (ref->getClass()->className() == u"java/lang/String") {
      types::JString out = utils::getStringValue(ref);
      std::cout << types::convertJString(out) << std::endl;
    } else {
      std::cout << value.asReference() << std::endl;
    }
  }

  return std::nullopt;
}

void Vm::registerNatives()
{
  // Temporary printing methods 'org.geethread.tests.basic.Printer'
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/Printer", u"println", u"(I)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/Printer", u"println", u"(C)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/Printer", u"println", u"(J)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/Printer", u"println", u"(F)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/Printer", u"println", u"(D)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/Printer", u"println", u"(Z)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/Printer", u"println", u"(Ljava/lang/String;)V"}, geevm_test_print);

  // java.lang.Object
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"hashCode", u"()I"}, java_lang_Object_hashCode);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"getClass", u"()Ljava/lang/Class;"}, java_lang_Object_getClass);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"wait", u"(J)V"}, java_lang_Object_wait);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"wait", u"()V"}, java_lang_Object_wait);

  // java.lang.System
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/System", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/System", u"initProperties", u"(Ljava/util/Properties;)Ljava/util/Properties;"},
                                      java_lang_System_initProperties);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/System", u"arraycopy", u"(Ljava/lang/Object;ILjava/lang/Object;II)V"},
                                      java_lang_System_arraycopy);

  // java.lang.Class
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"getPrimitiveClass", u"(Ljava/lang/String;)Ljava/lang/Class;"},
                                      java_lang_Class_getPrimitiveClass);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"desiredAssertionStatus0", u"(Ljava/lang/Class;)Z"},
                                      java_lang_Class_desiredAssertionStatus0);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"getName0", u"()Ljava/lang/String;"}, java_lang_Class_getName0);
  mNativeMethods.registerNativeMethod(
      ClassNameAndDescriptor{u"java/lang/Class", u"forName0", u"(Ljava/lang/String;ZLjava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/Class;"},
      java_lang_Class_forName0);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"getDeclaredFields0", u"(Z)[Ljava/lang/reflect/Field;"},
                                      java_lang_Class_getDeclaredFields0);

  // java.lang.ClassLoader
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/ClassLoader", u"registerNatives", u"()V"}, noop);

  // java.lang.String
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/String", u"intern", u"()Ljava/lang/String;"}, java_lang_String_intern);

  // java.lang.Thread
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"currentThread", u"()Ljava/lang/Thread;"}, java_lang_Thread_currentThread);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"setPriority0", u"(I)V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"isAlive", u"()Z"}, java_lang_Thread_isAlive);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"start0", u"()V"}, java_lang_Thread_start0);

  // java.lang.Float
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Float", u"floatToRawIntBits", u"(F)I"}, java_lang_Float_floatToRawIntBits);

  // java.lang.Double
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Double", u"doubleToRawLongBits", u"(D)J"}, java_lang_Double_doubleToRawIntBits);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Double", u"longBitsToDouble", u"(J)D"}, java_lang_Double_longBitsToDouble);

  // sun.misc.VM
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/misc/VM", u"initialize", u"()V"}, noop);

  // sun.misc.Unsafe
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/misc/Unsafe", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/misc/Unsafe", u"arrayBaseOffset", u"(Ljava/lang/Class;)I"}, sun_misc_Unsafe_arrayBaseOffset);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/misc/Unsafe", u"arrayIndexScale", u"(Ljava/lang/Class;)I"}, sun_misc_Unsafe_arrayBaseOffset);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/misc/Unsafe", u"addressSize", u"()I"}, sun_misc_Unsafe_arrayBaseOffset);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/misc/Unsafe", u"objectFieldOffset", u"(Ljava/lang/reflect/Field;)J"},
                                      sun_misc_Unsafe_objectFieldOffset);
  // mNativeMethods.registerNativeMethod(
  //     ClassNameAndDescriptor{u"sun/misc/Unsafe", u"compareAndSwapObject", u"(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z"},
  //     sun_misc_Unsafe_compareAndSwapObject);

  // sun.reflect.Reflection
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/reflect/Reflection", u"getCallerClass", u"()Ljava/lang/Class;"},
                                      sun_reflect_Reflection_getCallerClass);

  // java.io.FileInputStream
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/io/FileInputStream", u"initIDs", u"()V"}, noop);

  // java.io.FileOutputStream
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/io/FileOutputStream", u"initIDs", u"()V"}, noop);

  // java.io.FileDescriptor
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/io/FileDescriptor", u"initIDs", u"()V"}, noop);

  // java.security.AccessController
  mNativeMethods.registerNativeMethod(
      ClassNameAndDescriptor{u"java/security/AccessController", u"doPrivileged", u"(Ljava/security/PrivilegedExceptionAction;)Ljava/lang/Object;"},
      java_security_AccessController_doPrivileged);
  mNativeMethods.registerNativeMethod(
      ClassNameAndDescriptor{u"java/security/AccessController", u"doPrivileged", u"(Ljava/security/PrivilegedAction;)Ljava/lang/Object;"},
      java_security_AccessController_doPrivileged);
  mNativeMethods.registerNativeMethod(
      ClassNameAndDescriptor{u"java/security/AccessController", u"getStackAccessControlContext", u"()Ljava/security/AccessControlContext;"}, return_nullptr);

  // java.lang.Throwable
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Throwable", u"fillInStackTrace", u"(I)Ljava/lang/Throwable;"},
                                      java_lang_Throwable_fillInStackTrace);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Throwable", u"getStackTraceDepth", u"()I"}, java_lang_Throwable_getStackTraceDepth);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Throwable", u"getStackTraceElement", u"(I)Ljava/lang/StackTraceElement;"},
                                      java_lang_Throwable_getStackTraceElement);
}

std::optional<Value> noop(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return std::nullopt;
}

std::optional<Value> return_nullptr(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Reference(nullptr);
}

std::optional<Value> java_lang_Object_hashCode(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* objectRef = args[0].asReference();
  return Value::Int(static_cast<int32_t>(std::hash<Instance*>{}(objectRef)));
}

std::optional<Value> java_lang_Object_getClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* objectRef = args[0].asReference();
  Instance* klass = objectRef->getClass()->classInstance();

  return Value::Reference(klass);
}

std::optional<Value> java_lang_Object_wait(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return std::nullopt;
}

std::optional<Value> java_lang_Class_desiredAssertionStatus0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Int(0);
}

std::optional<Value> java_lang_Class_getPrimitiveClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto stringObject = args[0].asReference();
  auto charArray = stringObject->getFieldValue(u"value", u"[C").asReference()->asArrayInstance();

  types::JString buffer;
  for (Value v : charArray->contents()) {
    buffer += v.asChar();
  }

  if (buffer == u"float") {
    auto klass = thread.resolveClass(u"java/lang/Float");
    return Value::Reference((*klass)->classInstance());
  } else if (buffer == u"double") {
    auto klass = thread.resolveClass(u"java/lang/Double");
    return Value::Reference((*klass)->classInstance());
  } else if (buffer == u"int") {
    auto klass = thread.resolveClass(u"java/lang/Integer");
    return Value::Reference((*klass)->classInstance());
  } else {
    assert(false && "Unknown primitive class");
  }
}

std::optional<Value> java_lang_Class_getName0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  ClassInstance* cls = args[0].asReference()->asClassInstance();
  assert(cls != nullptr);

  auto name = cls->target()->className();
  std::ranges::replace(name, u'/', u'.');

  Instance* str = thread.heap().intern(name);

  return Value::Reference(str);
}

std::optional<Value> java_lang_Class_forName0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* name = args[0].asReference();
  int32_t initialize = args[1].asInt();
  Instance* classLoader = args[2].asReference();

  assert(classLoader == nullptr && "TODO: Support non-boostrap classloader");
  auto charArray = name->getFieldValue(u"value", u"[C").asReference()->asArrayInstance();

  types::JString nameStr = u"";
  for (Value value : charArray->contents()) {
    nameStr += value.asChar();
  }

  auto loaded = thread.resolveClass(nameStr);
  if (!loaded) {
    // TODO: throw exception
  }
  return Value::Reference((*loaded)->classInstance());
}

std::optional<Value> java_lang_Class_getDeclaredFields0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto fieldCls = thread.resolveClass(u"java/lang/reflect/Field");
  assert(fieldCls.has_value());

  Instance* clsInstance = args[0].asReference();
  bool isPublicOnly = args[1].asInt() == 1;

  auto klass = clsInstance->asClassInstance()->target();

  std::vector<Instance*> fields;
  for (auto& [nameAndDescriptor, field] : klass->fields()) {
    if (!isPublicOnly || field->isPublic()) {
      Instance* fieldInstance = thread.heap().allocate((*fieldCls)->asInstanceClass());
      fieldInstance->setFieldValue(u"name", u"Ljava/lang/String;", Value::Reference(thread.heap().intern(field->name())));
      fieldInstance->setFieldValue(u"modifiers", u"I", Value::Int(static_cast<int32_t>(field->accessFlags())));

      fields.push_back(fieldInstance);
    }
  }

  ArrayInstance* fieldsArray = thread.heap().allocateArray((*thread.resolveClass(u"[Ljava/lang/reflect/Field;"))->asArrayClass(), fields.size());
  for (int i = 0; i < fields.size(); i++) {
    fieldsArray->setArrayElement(i, Value::Reference(fields.at(i)));
  }

  return Value::Reference(fieldsArray);
}

std::optional<Value> java_lang_Float_floatToRawIntBits(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Int(std::bit_cast<int32_t>(args[0].asFloat()));
}

std::optional<Value> java_lang_Double_doubleToRawIntBits(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Long(std::bit_cast<int64_t>(args[0].asDouble()));
}

std::optional<Value> java_lang_Double_longBitsToDouble(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Double(std::bit_cast<double>(args[0].asLong()));
}

std::optional<Value> java_lang_System_initProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Reference(args[0].asReference());
}

std::optional<Value> java_lang_System_arraycopy(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* source = args[0].asReference();
  int32_t sourcePos = args[1].asInt();
  ArrayInstance* target = args[2].asReference()->asArrayInstance();
  int32_t targetPos = args[3].asInt();
  int32_t len = args[4].asInt();

  auto& sourceArray = source->asArrayInstance()->contents();
  assert(sourcePos < sourceArray.size());
  assert(target != nullptr);

  for (int32_t i = 0; i < len; i++) {
    Value value = sourceArray.at(sourcePos + i);
    target->setArrayElement(targetPos + i, value);
  }

  return std::nullopt;
}

std::optional<Value> sun_misc_Unsafe_arrayBaseOffset(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Int(0);
}

std::optional<Value> sun_misc_Unsafe_objectFieldOffset(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  // TODO
  return Value::Long(0);
}

std::optional<Value> sun_misc_Unsafe_compareAndSwapObject(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* obj = args[1].asReference();
  int64_t offset = args[2].asLong();
  // 'offset' is category 2, so we skip an index
  Instance* expected = args[4].asReference();
  Instance* target = args[5].asReference();

  // FIXME
  return Value::Int(1);
}

std::optional<Value> sun_reflect_Reflection_getCallerClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  // Returns the class of the caller of the method calling this method ignoring frames associated with java.lang.reflect.Method.invoke() and its implementation.
  CallFrame* previous = frame.previous()->previous();

  return Value::Reference(previous->currentClass()->classInstance());
}

std::optional<Value> java_security_AccessController_doPrivileged(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto actionCls = thread.resolveClass(u"java/security/PrivilegedExceptionAction");
  // TODO: Check loaded

  Instance* target = args[0].asReference();
  auto runMethod = target->getClass()->getMethod(u"run", u"()Ljava/lang/Object;");
  assert(runMethod.has_value());

  return thread.executeCall(*runMethod, {Value::Reference(target)});
}

std::optional<Value> java_lang_Thread_currentThread(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Reference(thread.instance());
}

std::optional<Value> java_lang_Thread_isAlive(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* threadRef = args[0].asReference();
  int64_t eetop = threadRef->getFieldValue(u"eetop", u"J").asLong();

  if (eetop == 0) {
    return Value::Int(0);
  }

  int ret = pthread_kill((pthread_t)eetop, 0);
  if (ret == 0) {
    return Value::Int(1);
  } else if (ret == ESRCH) {
    return Value::Int(0);
  }

  assert(false && "Impossible");
}

std::optional<Value> java_lang_Thread_start0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* threadRef = args[0].asReference();
  auto target = threadRef->getClass()->getVirtualMethod(u"run", u"()V");
  assert(target.has_value());

  if ((*target)->getClass()->className() == u"java/lang/ref/Reference$ReferenceHandler") {
    return std::nullopt;
  }

  auto targetThread = std::make_unique<JavaThread>(thread.vm());
  targetThread->setThreadInstance(threadRef);

  targetThread->start(*target, {});

  return std::nullopt;
}

std::optional<Value> java_lang_Throwable_fillInStackTrace(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* exceptionInstance = args[0].asReference();

  auto array = thread.createStackTrace();
  // exceptionInstance->setFieldValue(u"stackTrace", u"[Ljava/lang/StackTraceElement;", Value::Reference(array));
  exceptionInstance->setFieldValue(u"stackTrace", u"[Ljava/lang/StackTraceElement;", Value::Reference(nullptr));
  exceptionInstance->setFieldValue(u"backtrace", u"Ljava/lang/Object;", Value::Reference(array));

  return Value::Reference(exceptionInstance);
}

std::optional<Value> java_lang_Throwable_getStackTraceDepth(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* exceptionInstance = args[0].asReference();
  auto backtrace = exceptionInstance->getFieldValue(u"backtrace", u"Ljava/lang/Object;");

  if (backtrace.asReference() == nullptr) {
    return Value::Int(0);
  }

  return Value::Int(backtrace.asReference()->asArrayInstance()->length());
}

std::optional<Value> java_lang_Throwable_getStackTraceElement(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* exceptionInstance = args[0].asReference();
  int32_t index = args[1].asInt();

  auto backtrace = exceptionInstance->getFieldValue(u"backtrace", u"Ljava/lang/Object;");
  auto elem = backtrace.asReference()->asArrayInstance()->getArrayElement(index);

  assert(elem.has_value());

  return Value::Reference(elem->asReference());
}

std::optional<Value> java_lang_String_intern(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* self = args.at(0).asReference();
  auto& charArray = self->getFieldValue(u"value", u"[C").asReference()->asArrayInstance()->contents();

  types::JString str = u"";
  for (auto value : charArray) {
    str += value.asChar();
  }

  Instance* interned = thread.heap().intern(str);

  return Value::Reference(interned);
}

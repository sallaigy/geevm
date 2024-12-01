#include "vm/NativeMethods.h"

#include "VmUtils.h"
#include "vm/Frame.h"
#include "vm/Thread.h"
#include "vm/Vm.h"

#include <algorithm>
#include <csignal>
#include <iostream>
#include <unordered_set>
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
static std::optional<Value> return_false(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from<int32_t>(0);
}

static std::optional<Value> java_lang_Object_hashCode(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Object_getClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Object_wait(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_System_initProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_System_arraycopy(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_System_nanoTime(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Class_getPrimitiveClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_isPrimitive(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_desiredAssertionStatus0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_getName0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_forName0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_initClassName(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_getDeclaredFields0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Float_floatToRawIntBits(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Double_doubleToRawIntBits(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Double_longBitsToDouble(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> sun_misc_Unsafe_arrayBaseOffset(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> sun_misc_Unsafe_arrayIndexScale(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> sun_misc_Unsafe_objectFieldOffset(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> sun_misc_Unsafe_storeFence(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> jdk_internal_util_SystemProps_Raw_platformProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> jdk_internal_util_SystemProps_Raw_vmProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> jdk_internal_misc_CDS_getRandomSeedForDumping(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> jdk_internal_misc_Unsafe_compareAndSetInt(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> jdk_internal_misc_Unsafe_compareAndSetReference(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> jdk_internal_misc_Unsafe_getReferenceVolatile(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> sun_reflect_Reflection_getCallerClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_security_AccessController_doPrivileged(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Thread_currentThread(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Thread_isAlive(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Thread_start0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Runtime_availableProcessors(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Runtime_maxMemory(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Throwable_fillInStackTrace(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Throwable_getStackTraceDepth(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Throwable_getStackTraceElement(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_StackTraceElement_initStackTraceElements(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_String_intern(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_StringUTF16_isBigEndian(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> geevm_test_print(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Value value = args[0];

  const FieldType& type = frame.currentMethod()->descriptor().parameters().at(0);

  if (auto primitiveType = type.asPrimitive(); primitiveType) {
    switch (*primitiveType) {
      case PrimitiveType::Byte: std::cout << value.get<std::int32_t>() << std::endl; break;
      case PrimitiveType::Char: {
        char16_t charValue = static_cast<char16_t>(value.get<std::int32_t>());
        std::cout << types::convertJString(types::JString{charValue}) << std::endl;
        break;
      }
      case PrimitiveType::Double: std::cout << value.get<double>() << std::endl; break;
      case PrimitiveType::Float: std::cout << value.get<float>() << std::endl; break;
      case PrimitiveType::Int: std::cout << value.get<std::int32_t>() << std::endl; break;
      case PrimitiveType::Long: std::cout << value.get<std::int64_t>() << std::endl; break;
      case PrimitiveType::Short: std::cout << value.get<std::int32_t>() << std::endl; break;
      case PrimitiveType::Boolean: std::cout << (value.get<std::int32_t>() == 1 ? "true" : "false") << std::endl; break;
    }
  } else {
    Instance* ref = value.get<Instance*>();
    if (ref->getClass()->className() == u"java/lang/String") {
      types::JString out = utils::getStringValue(ref);
      std::cout << types::convertJString(out) << std::endl;
    } else {
      std::cout << ref << std::endl;
    }
  }

  return std::nullopt;
}

void Vm::registerNatives()
{
  // Temporary printing methods 'org.geethread.tests.basic.Printer'
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/util/Printer", u"println", u"(I)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/util/Printer", u"println", u"(C)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/util/Printer", u"println", u"(J)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/util/Printer", u"println", u"(F)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/util/Printer", u"println", u"(D)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/util/Printer", u"println", u"(Z)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/util/Printer", u"println", u"(Ljava/lang/String;)V"}, geevm_test_print);

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
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/System", u"nanoTime", u"()J"}, java_lang_System_nanoTime);

  // java.lang.Class
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"getPrimitiveClass", u"(Ljava/lang/String;)Ljava/lang/Class;"},
                                      java_lang_Class_getPrimitiveClass);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"isPrimitive", u"()Z"}, java_lang_Class_isPrimitive);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"desiredAssertionStatus0", u"(Ljava/lang/Class;)Z"},
                                      java_lang_Class_desiredAssertionStatus0);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"getName0", u"()Ljava/lang/String;"}, java_lang_Class_getName0);
  mNativeMethods.registerNativeMethod(
      ClassNameAndDescriptor{u"java/lang/Class", u"forName0", u"(Ljava/lang/String;ZLjava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/Class;"},
      java_lang_Class_forName0);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"getDeclaredFields0", u"(Z)[Ljava/lang/reflect/Field;"},
                                      java_lang_Class_getDeclaredFields0);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"initClassName", u"()Ljava/lang/String;"}, java_lang_Class_initClassName);

  // java.lang.ClassLoader
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/ClassLoader", u"registerNatives", u"()V"}, noop);

  // java.lang.String
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/String", u"intern", u"()Ljava/lang/String;"}, java_lang_String_intern);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/StringUTF16", u"isBigEndian", u"()Z"}, java_lang_StringUTF16_isBigEndian);

  // java.lang.Thread
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"currentThread", u"()Ljava/lang/Thread;"}, java_lang_Thread_currentThread);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"setPriority0", u"(I)V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"isAlive", u"()Z"}, java_lang_Thread_isAlive);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"start0", u"()V"}, java_lang_Thread_start0);

  // java.lang.Runtime
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Runtime", u"availableProcessors", u"()I"}, java_lang_Runtime_availableProcessors);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Runtime", u"maxMemory", u"()J"}, java_lang_Runtime_maxMemory);

  // java.lang.Float
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Float", u"floatToRawIntBits", u"(F)I"}, java_lang_Float_floatToRawIntBits);

  // java.lang.Double
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Double", u"doubleToRawLongBits", u"(D)J"}, java_lang_Double_doubleToRawIntBits);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Double", u"longBitsToDouble", u"(J)D"}, java_lang_Double_longBitsToDouble);

  // sun.misc.VM
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/VM", u"initialize", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/CDS", u"isDumpingClassList0", u"()Z"}, return_false);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/CDS", u"isDumpingArchive0", u"()Z"}, return_false);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/CDS", u"isSharingEnabled0", u"()Z"}, return_false);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/CDS", u"getRandomSeedForDumping", u"()J"},
                                      jdk_internal_misc_CDS_getRandomSeedForDumping);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/CDS", u"initializeFromArchive", u"(Ljava/lang/Class;)V"}, noop);

  // sun.misc.Unsafe
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/misc/Unsafe", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"arrayBaseOffset0", u"(Ljava/lang/Class;)I"},
                                      sun_misc_Unsafe_arrayBaseOffset);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"arrayIndexScale0", u"(Ljava/lang/Class;)I"},
                                      sun_misc_Unsafe_arrayIndexScale);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"arrayIndexScale", u"(Ljava/lang/Class;)I"},
                                      sun_misc_Unsafe_arrayIndexScale);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"addressSize", u"()I"}, sun_misc_Unsafe_arrayBaseOffset);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/util/SystemProps$Raw", u"platformProperties", u"()[Ljava/lang/String;"},
                                      jdk_internal_util_SystemProps_Raw_platformProperties);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/util/SystemProps$Raw", u"vmProperties", u"()[Ljava/lang/String;"},
                                      jdk_internal_util_SystemProps_Raw_vmProperties);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"objectFieldOffset1", u"(Ljava/lang/Class;Ljava/lang/String;)J"},
                                      sun_misc_Unsafe_objectFieldOffset);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"storeFence", u"()V"}, sun_misc_Unsafe_storeFence);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"compareAndSetInt", u"(Ljava/lang/Object;JII)Z"},
                                      jdk_internal_misc_Unsafe_compareAndSetInt);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"compareAndSetLong", u"(Ljava/lang/Object;JJJ)Z"},
                                      jdk_internal_misc_Unsafe_compareAndSetInt);
  mNativeMethods.registerNativeMethod(
      ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"compareAndSetReference", u"(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z"},
      jdk_internal_misc_Unsafe_compareAndSetReference);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/Unsafe", u"getReferenceVolatile", u"(Ljava/lang/Object;J)Ljava/lang/Object;"},
                                      jdk_internal_misc_Unsafe_getReferenceVolatile);
  // mNativeMethods.registerNativeMethod(
  //     ClassNameAndDescriptor{u"sun/misc/Unsafe", u"compareAndSwapObject", u"(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z"},
  //     sun_misc_Unsafe_compareAndSwapObject);

  // sun.reflect.Reflection
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/reflect/Reflection", u"getCallerClass", u"()Ljava/lang/Class;"},
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

  // java.lang.StackTraceElement
  mNativeMethods.registerNativeMethod(
      ClassNameAndDescriptor{u"java/lang/StackTraceElement", u"initStackTraceElements", u"([Ljava/lang/StackTraceElement;Ljava/lang/Throwable;)V"},
      java_lang_StackTraceElement_initStackTraceElements);
}

std::optional<Value> noop(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return std::nullopt;
}

std::optional<Value> return_nullptr(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from<Instance*>(nullptr);
}

std::optional<Value> java_lang_Object_hashCode(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* objectRef = args[0].get<Instance*>();
  return Value::from<int32_t>(static_cast<int32_t>(std::hash<Instance*>{}(objectRef)));
}

std::optional<Value> java_lang_Object_getClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* objectRef = args[0].get<Instance*>();
  Instance* klass = objectRef->getClass()->classInstance();

  return Value::from(klass);
}

std::optional<Value> java_lang_Object_wait(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return std::nullopt;
}

std::optional<Value> java_lang_Class_desiredAssertionStatus0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from<int32_t>(0);
}

std::optional<Value> java_lang_Class_getPrimitiveClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto stringObject = args[0].get<Instance*>();
  types::JString buffer = utils::getStringValue(stringObject);

  if (buffer == u"float") {
    auto klass = thread.resolveClass(u"java/lang/Float");
    return Value::from((*klass)->classInstance());
  } else if (buffer == u"double") {
    auto klass = thread.resolveClass(u"java/lang/Double");
    return Value::from((*klass)->classInstance());
  } else if (buffer == u"int") {
    auto klass = thread.resolveClass(u"java/lang/Integer");
    return Value::from((*klass)->classInstance());
  } else if (buffer == u"byte") {
    auto klass = thread.resolveClass(u"java/lang/Byte");
    return Value::from((*klass)->classInstance());
  } else if (buffer == u"char") {
    auto klass = thread.resolveClass(u"java/lang/Character");
    return Value::from((*klass)->classInstance());
  } else {
    assert(false && "Unknown primitive class");
  }
}

std::optional<Value> java_lang_Class_isPrimitive(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto classObject = args[0].get<Instance*>()->asClassInstance();
  const types::JString& className = classObject->target()->className();

  static std::unordered_set<types::JString> klassNames = {
      u"java/lang/Boolean", u"java/Lang/Float", u"java/lang/Double",  u"java/lang/Byte",
      u"java/lang/Char",    u"java/lang/Short", u"java/lang/Integer", u"java/lang/Long",
  };

  return klassNames.find(className) == klassNames.end() ? Value::from<int32_t>(0) : Value::from<int32_t>(1);
}

std::optional<Value> java_lang_Class_getName0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  ClassInstance* cls = args[0].get<Instance*>()->asClassInstance();
  assert(cls != nullptr);

  auto name = cls->target()->className();
  std::ranges::replace(name, u'/', u'.');

  Instance* str = thread.heap().intern(name);

  return Value::from(str);
}

std::optional<Value> java_lang_Class_forName0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* name = args[0].get<Instance*>();
  int32_t initialize = args[1].get<int32_t>();
  Instance* classLoader = args[2].get<Instance*>();

  assert(classLoader == nullptr && "TODO: Support non-boostrap classloader");
  types::JString nameStr = utils::getStringValue(name);

  auto loaded = thread.resolveClass(nameStr);
  if (!loaded) {
    // TODO: throw exception
  }
  return Value::from((*loaded)->classInstance());
}

std::optional<Value> java_lang_Class_initClassName(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  ClassInstance* cls = args[0].get<Instance*>()->asClassInstance();
  assert(cls != nullptr);

  auto name = cls->target()->className();
  std::ranges::replace(name, u'/', u'.');

  Instance* str = thread.heap().intern(name);

  return Value::from(str);
}

std::optional<Value> java_lang_Class_getDeclaredFields0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto fieldCls = thread.resolveClass(u"java/lang/reflect/Field");
  assert(fieldCls.has_value());

  Instance* clsInstance = args[0].get<Instance*>();
  bool isPublicOnly = args[1].get<int32_t>() == 1;

  auto klass = clsInstance->asClassInstance()->target();

  std::vector<Instance*> fields;
  for (auto& [nameAndDescriptor, field] : klass->fields()) {
    if (!isPublicOnly || field->isPublic()) {
      Instance* fieldInstance = thread.heap().allocate((*fieldCls)->asInstanceClass());
      fieldInstance->setFieldValue(u"name", u"Ljava/lang/String;", thread.heap().intern(field->name()));
      fieldInstance->setFieldValue(u"modifiers", u"I", static_cast<int32_t>(field->accessFlags()));

      fields.push_back(fieldInstance);
    }
  }

  ArrayInstance* fieldsArray = thread.heap().allocateArray((*thread.resolveClass(u"[Ljava/lang/reflect/Field;"))->asArrayClass(), fields.size());
  for (int i = 0; i < fields.size(); i++) {
    fieldsArray->setArrayElement(i, fields.at(i));
  }

  return Value::from<Instance*>(fieldsArray);
}

std::optional<Value> java_lang_Float_floatToRawIntBits(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from(std::bit_cast<int32_t>(args[0].get<float>()));
}

std::optional<Value> java_lang_Double_doubleToRawIntBits(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from(std::bit_cast<int64_t>(args[0].get<double>()));
}

std::optional<Value> java_lang_Double_longBitsToDouble(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from(std::bit_cast<double>(args[0].get<int64_t>()));
}

std::optional<Value> java_lang_System_initProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from(args[0].get<Instance*>());
}

std::optional<Value> java_lang_System_arraycopy(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* source = args[0].get<Instance*>();
  int32_t sourcePos = args[1].get<int32_t>();
  ArrayInstance* target = args[2].get<Instance*>()->asArrayInstance();
  int32_t targetPos = args[3].get<int32_t>();
  int32_t len = args[4].get<int32_t>();

  auto sourceArray = source->asArrayInstance();
  assert(sourcePos < sourceArray->length());
  assert(target != nullptr);

  for (int32_t i = 0; i < len; i++) {
    Value value = *sourceArray->getArrayElement(sourcePos + i);
    target->setArrayElement(targetPos + i, value);
  }

  return std::nullopt;
}

std::optional<Value> java_lang_System_nanoTime(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto now = std::chrono::high_resolution_clock::now();
  auto duration = now.time_since_epoch();

  return Value::from<int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
}

std::optional<Value> sun_misc_Unsafe_arrayBaseOffset(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  ClassInstance* cls = args[1].get<Instance*>()->asClassInstance();
  assert(cls->target()->isArrayType());

  ArrayClass* arrayClass = cls->target()->asArrayClass();

  return Value::from<int32_t>(sizeof(ArrayInstance));
}

std::optional<Value> sun_misc_Unsafe_arrayIndexScale(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  ClassInstance* clsInstance = args[1].get<Instance*>()->asClassInstance();
  ArrayClass* arrayClass = clsInstance->target()->asArrayClass();

  return Value::from<int32_t>(sizeof(Value));
}

std::optional<Value> sun_misc_Unsafe_objectFieldOffset(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  // TODO
  return Value::from<int64_t>(0);
}

std::optional<Value> sun_misc_Unsafe_storeFence(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  std::atomic_thread_fence(std::memory_order_acquire);
  return std::nullopt;
}

std::optional<Value> jdk_internal_misc_Unsafe_compareAndSetInt(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* object = args[1].get<Instance*>();
  int64_t offset = args[2].get<int64_t>();
  Value* expected = const_cast<Value*>(&args[4]);
  Value* desired = const_cast<Value*>(&args[5]);

  Value* target = reinterpret_cast<Value*>(reinterpret_cast<char*>(object) + offset);
  std::atomic_ref<Value*> atomicRef(target);

  bool success = atomicRef.compare_exchange_strong(expected, desired, std::memory_order_seq_cst);
  return success ? Value::from<int32_t>(1) : Value::from<int32_t>(0);
}

std::optional<Value> jdk_internal_misc_Unsafe_compareAndSetReference(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* object = args[1].get<Instance*>();
  int64_t offset = args[2].get<int64_t>();
  Value* expected = const_cast<Value*>(&args[4]);
  Value* desired = const_cast<Value*>(&args[5]);

  Value* target = reinterpret_cast<Value*>(reinterpret_cast<char*>(object) + offset);
  std::atomic_ref<Value*> atomicRef(target);

  bool success = atomicRef.compare_exchange_strong(expected, desired, std::memory_order_seq_cst);
  return success ? Value::from<int32_t>(1) : Value::from<int32_t>(0);
}

std::optional<Value> jdk_internal_misc_Unsafe_getReferenceVolatile(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* object = args[1].get<Instance*>();
  int64_t offset = args[2].get<int64_t>();

  Value* target = reinterpret_cast<Value*>(reinterpret_cast<char*>(object) + offset);
  std::atomic_ref<Value*> atomicRef(target);

  return *atomicRef.load();
}

std::optional<Value> sun_reflect_Reflection_getCallerClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  // Returns the class of the caller of the method calling this method ignoring frames associated with java.lang.reflect.Method.invoke() and its implementation.
  CallFrame* previous = frame.previous()->previous();

  return Value::from(previous->currentClass()->classInstance());
}

std::optional<Value> java_security_AccessController_doPrivileged(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto actionCls = thread.resolveClass(u"java/security/PrivilegedExceptionAction");
  // TODO: Check loaded

  Instance* target = args[0].get<Instance*>();
  auto runMethod = target->getClass()->getMethod(u"run", u"()Ljava/lang/Object;");
  assert(runMethod.has_value());

  return thread.executeCall(*runMethod, {Value::from(target)});
}

std::optional<Value> java_lang_Thread_currentThread(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from(thread.instance());
}

std::optional<Value> java_lang_Thread_isAlive(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* threadRef = args[0].get<Instance*>();
  int64_t eetop = threadRef->getFieldValue<int64_t>(u"eetop", u"J");

  if (eetop == 0) {
    return Value::from(0);
  }

  int ret = pthread_kill((pthread_t)eetop, 0);
  if (ret == 0) {
    return Value::from(1);
  } else if (ret == ESRCH) {
    return Value::from(0);
  }

  assert(false && "Impossible");
}

std::optional<Value> java_lang_Thread_start0(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* threadRef = args[0].get<Instance*>();
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

std::optional<Value> java_lang_Runtime_availableProcessors(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from(1);
}

std::optional<Value> java_lang_Runtime_maxMemory(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from(2000);
}

std::optional<Value> java_lang_Throwable_fillInStackTrace(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* exceptionInstance = args[0].get<Instance*>();

  auto array = thread.createStackTrace();
  // exceptionInstance->setFieldValue(u"stackTrace", u"[Ljava/lang/StackTraceElement;", Value::Reference(array));
  exceptionInstance->setFieldValue<Instance*>(u"stackTrace", u"[Ljava/lang/StackTraceElement;", nullptr);
  exceptionInstance->setFieldValue<Instance*>(u"backtrace", u"Ljava/lang/Object;", array);
  exceptionInstance->setFieldValue<int32_t>(u"depth", u"I", array->asArrayInstance()->length());

  return Value::from(exceptionInstance);
}

std::optional<Value> java_lang_Throwable_getStackTraceDepth(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* exceptionInstance = args[0].get<Instance*>();
  auto backtrace = exceptionInstance->getFieldValue<Instance*>(u"backtrace", u"Ljava/lang/Object;");

  if (backtrace == nullptr) {
    return Value::from(0);
  }

  return Value::from(backtrace->asArrayInstance()->length());
}

std::optional<Value> java_lang_Throwable_getStackTraceElement(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* exceptionInstance = args[0].get<Instance*>();
  int32_t index = args[1].get<int32_t>();

  auto backtrace = exceptionInstance->getFieldValue<Instance*>(u"backtrace", u"Ljava/lang/Object;");
  auto elem = backtrace->asArrayInstance()->getArrayElement<Instance*>(index);

  assert(elem.has_value());

  return Value::from(*elem);
}

std::optional<Value> java_lang_StackTraceElement_initStackTraceElements(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  ArrayInstance* stackTraceArray = args[0].get<Instance*>()->asArrayInstance();
  Instance* throwable = args[1].get<Instance*>();

  auto storedBackTrace = throwable->getFieldValue<Instance*>(u"backtrace", u"Ljava/lang/Object;")->asArrayInstance();

  assert(storedBackTrace->length() == stackTraceArray->length());

  for (int32_t i = 0; i < stackTraceArray->length(); i++) {
    stackTraceArray->asArrayInstance()->setArrayElement(i, *storedBackTrace->getArrayElement(i));
  }

  return std::nullopt;
}

std::optional<Value> java_lang_String_intern(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* self = args.at(0).get<Instance*>();

  types::JString str = utils::getStringValue(self);
  Instance* interned = thread.heap().intern(str);

  return Value::from(interned);
}

std::optional<Value> java_lang_StringUTF16_isBigEndian(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from<int32_t>(0);
}

std::optional<Value> jdk_internal_util_SystemProps_Raw_platformProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto stringArrayCls = thread.resolveClass(u"[Ljava/lang/String;");

  auto rawPropsCls = *thread.resolveClass(u"jdk/internal/util/SystemProps$Raw");
  auto arrayLength = rawPropsCls->getStaticFieldValue<int32_t>(u"FIXED_LENGTH", u"I");

  ArrayInstance* propsArray = thread.heap().allocateArray((*stringArrayCls)->asArrayClass(), arrayLength);

  return Value::from<Instance*>(propsArray);
}

std::optional<Value> jdk_internal_util_SystemProps_Raw_vmProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto stringArrayCls = thread.resolveClass(u"[Ljava/lang/String;");

  auto rawPropsCls = *thread.resolveClass(u"jdk/internal/util/SystemProps$Raw");
  auto arrayLength = 2;

  ArrayInstance* propsArray = thread.heap().allocateArray((*stringArrayCls)->asArrayClass(), arrayLength);
  propsArray->setArrayElement(0, thread.heap().intern(u"java.home"));
  propsArray->setArrayElement(1, thread.heap().intern(u"/home/gyula/projects/geevm/cmake-build-debug/jdk17"));

  return Value::from<Instance*>(propsArray);
}

std::optional<Value> jdk_internal_misc_CDS_getRandomSeedForDumping(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from<int64_t>(0);
}

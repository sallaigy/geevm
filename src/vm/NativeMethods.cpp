#include "vm/NativeMethods.h"

#include "JniImplementation.h"
#include "VmUtils.h"
#include "common/DynamicLibrary.h"
#include "vm/Frame.h"
#include "vm/Thread.h"
#include "vm/Vm.h"

#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <ffi.h>
#include <filesystem>
#include <format>
#include <iostream>
#include <jni.h>
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

struct JObjectAdaptor : public _jobject
{
  Instance* actualObject;
};

struct JClassAdaptor : public _jclass
{
  ClassInstance* actualClass;
};

std::optional<NativeMethod> NativeMethodRegistry::getNativeMethod(const JMethod* method)
{
  auto dl = DynamicLibrary::create();

  types::JString name = u"Java_";
  std::ranges::replace_copy(method->getClass()->className(), std::back_inserter<std::u16string>(name), u'/', u'_');
  name += u"_" + method->name();

  auto nameStr = types::convertJString(name);
  void* symbol = dl->findSymbol(nameStr.c_str());

  if (symbol != nullptr) {
    return NativeMethod{method, symbol};
  }

  return std::nullopt;
}

std::optional<Value> NativeMethod::invoke(JavaThread& thread, const std::vector<Value>& args)
{
  ffi_cif cif;
  std::vector<ffi_type*> argTypes;

  // The first parameter is JNIEnv*, a pointer
  argTypes.push_back(&ffi_type_pointer);

  std::list<jvalue> argValues;
  size_t argIdx = 0;

  JniImplementation impl(thread);
  std::vector<void*> actualArgs;

  JNIEnv* env = impl.getEnv();
  actualArgs.push_back(&env);

  if (mMethod->isStatic()) {
    auto klass = mMethod->getClass()->classInstance()->asClassInstance();
    argTypes.push_back(&ffi_type_pointer);
    argValues.push_back(jvalue{.l = JniTranslate<ClassInstance*, jclass>{}(klass)});
    actualArgs.push_back(&argValues.back().l);
  } else {
    auto javaThis = args.at(0).get<Instance*>();
    argTypes.push_back(&ffi_type_pointer);
    argValues.push_back(jvalue{.l = JniTranslate<Instance*, jobject>{}(javaThis)});
    actualArgs.push_back(&argValues.back().l);
    argIdx = 1;
  }

  for (const FieldType& paramType : mMethod->descriptor().parameters()) {
    const auto& current = args.at(argIdx);
    argIdx++;
    if (auto primitive = paramType.asPrimitive(); primitive) {
      switch (*primitive) {
        case PrimitiveType::Byte: {
          argValues.push_back(jvalue{.b = current.get<int8_t>()});
          argTypes.push_back(&ffi_type_sint8);
          actualArgs.push_back(&argValues.back().b);
          break;
        }
        case PrimitiveType::Char: {
          argValues.push_back(jvalue{.c = std::bit_cast<uint16_t>(current.get<char16_t>())});
          argTypes.push_back(&ffi_type_uint16);
          actualArgs.push_back(&argValues.back().c);
          break;
        }
        case PrimitiveType::Float: {
          argValues.push_back(jvalue{.f = current.get<float>()});
          argTypes.push_back(&ffi_type_float);
          actualArgs.push_back(&argValues.back().f);
          break;
        }
        case PrimitiveType::Int: {
          argValues.push_back(jvalue{.i = current.get<int32_t>()});
          argTypes.push_back(&ffi_type_sint32);
          actualArgs.push_back(&argValues.back().i);
          break;
        }
        case PrimitiveType::Short: {
          argValues.push_back(jvalue{.s = current.get<int16_t>()});
          argTypes.push_back(&ffi_type_sint16);
          actualArgs.push_back(&argValues.back().s);
          break;
        }
        case PrimitiveType::Boolean: {
          bool boolValue = current.get<int32_t>() == 0 ? false : true;
          argValues.push_back(jvalue{.z = static_cast<uint8_t>(boolValue)});
          argTypes.push_back(&ffi_type_uint8);
          actualArgs.push_back(&argValues.back().z);
          break;
        }
        case PrimitiveType::Double: {
          argValues.push_back(jvalue{.d = current.get<double>()});
          argTypes.push_back(&ffi_type_double);
          actualArgs.push_back(&argValues.back().d);
          argIdx++;
          break;
        }
        case PrimitiveType::Long: {
          argValues.push_back(jvalue{.j = current.get<int64_t>()});
          argTypes.push_back(&ffi_type_sint64);
          actualArgs.push_back(&argValues.back().j);
          argIdx++;
          break;
        }
      }
    } else {
      // Must object or array reference
      argValues.push_back(jvalue{.l = JniTranslate<Instance*, jobject>{}(current.get<Instance*>())});
      argTypes.push_back(&ffi_type_pointer);
      actualArgs.push_back(&argValues.back().l);
    }
  }

  ffi_type* returnType;
  void* result;
  jvalue returnValue;
  if (mMethod->descriptor().returnType().isVoid()) {
    returnType = &ffi_type_void;
    result = nullptr;
  } else {
    if (auto primitive = mMethod->descriptor().returnType().getType().asPrimitive(); primitive) {
      switch (*primitive) {
        case PrimitiveType::Byte:
          returnType = &ffi_type_sint8;
          result = &returnValue.b;
          break;
        case PrimitiveType::Char:
          returnType = &ffi_type_uint16;
          result = &returnValue.c;
          break;
        case PrimitiveType::Double:
          returnType = &ffi_type_double;
          result = &returnValue.d;
          break;
        case PrimitiveType::Float:
          returnType = &ffi_type_float;
          result = &returnValue.f;
          break;
        case PrimitiveType::Int:
          returnType = &ffi_type_sint32;
          result = &returnValue.i;
          break;
        case PrimitiveType::Long:
          returnType = &ffi_type_sint64;
          result = &returnValue.j;
          break;
        case PrimitiveType::Short:
          returnType = &ffi_type_sint16;
          result = &returnValue.s;
          break;
        case PrimitiveType::Boolean:
          returnType = &ffi_type_uint8;
          result = &returnValue.z;
          break;
      }
    } else {
      returnType = &ffi_type_pointer;
      result = &returnValue.l;
    }
  }

  assert(actualArgs.size() == mMethod->descriptor().parameters().size() + 2);
  assert(actualArgs.size() == argTypes.size());

  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argTypes.size(), returnType, argTypes.data()) != FFI_OK) {
    thread.throwException(u"java/lang/RuntimeException", u"Failed to invoke nativ method");
    return std::nullopt;
  }

  ffi_call(&cif, FFI_FN(mHandle), result, actualArgs.data());

  if (mMethod->descriptor().returnType().isVoid()) {
    return std::nullopt;
  }

  if (auto primitive = mMethod->descriptor().returnType().getType().asPrimitive(); primitive) {
    switch (*primitive) {
      case PrimitiveType::Byte: return Value::from<int8_t>(returnValue.b);
      case PrimitiveType::Char: return Value::from<char16_t>(returnValue.c);
      case PrimitiveType::Double: return Value::from<double>(returnValue.d);
      case PrimitiveType::Float: return Value::from<float>(returnValue.f);
      case PrimitiveType::Int: return Value::from<int32_t>(returnValue.i);
      case PrimitiveType::Long: return Value::from<int64_t>(returnValue.j);
      case PrimitiveType::Short: return Value::from<int16_t>(returnValue.s);
      case PrimitiveType::Boolean: return Value::from<int32_t>(returnValue.z ? 1 : 0);
    }
    std::unreachable();
  } else {
    return Value::from<Instance*>(JniTranslate<jobject, Instance*>{}(returnValue.l));
  }
}

static std::optional<Value> noop(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> return_nullptr(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> return_false(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::from<int32_t>(0);
}

static std::optional<Value> java_lang_Object_wait(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> jdk_internal_util_SystemProps_Raw_platformProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> jdk_internal_util_SystemProps_Raw_vmProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> sun_reflect_Reflection_getCallerClass(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_security_AccessController_doPrivileged(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Throwable_fillInStackTrace(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Throwable_getStackTraceDepth(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Throwable_getStackTraceElement(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_StackTraceElement_initStackTraceElements(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_io_FileOutputStream_writeBytes(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args);

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
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"wait", u"(J)V"}, java_lang_Object_wait);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"wait", u"()V"}, java_lang_Object_wait);

  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/util/SystemProps$Raw", u"platformProperties", u"()[Ljava/lang/String;"},
                                      jdk_internal_util_SystemProps_Raw_platformProperties);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/util/SystemProps$Raw", u"vmProperties", u"()[Ljava/lang/String;"},
                                      jdk_internal_util_SystemProps_Raw_vmProperties);

  // sun.reflect.Reflection
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/reflect/Reflection", u"getCallerClass", u"()Ljava/lang/Class;"},
                                      sun_reflect_Reflection_getCallerClass);

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

  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"jdk/internal/misc/ScopedMemoryAccess", u"registerNatives", u"()V"}, noop);

  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/io/FileOutputStream", u"writeBytes", u"([BIIZ)V"}, java_io_FileOutputStream_writeBytes);
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

  static std::unordered_map<types::JStringRef, types::JStringRef> classNames = {
      {u"float", u"java/lang/Float"}, {u"double", u"java/lang/Double"}, {u"int", u"java/lang/Integer"},   {u"byte", u"java/lang/Byte"},
      {u"short", u"java/lang/Short"}, {u"long", u"java/lang/Long"},     {u"char", u"java/lang/Character"}};

  assert(classNames.contains(buffer));

  auto klass = thread.resolveClass(types::JString{classNames.at(buffer)});
  return Value::from((*klass)->classInstance());
}

std::optional<Value> java_lang_Class_isPrimitive(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto classObject = args[0].get<Instance*>()->asClassInstance();
  const types::JString& className = classObject->target()->className();

  static std::unordered_set<types::JString> klassNames = {
      u"java/lang/Boolean", u"java/Lang/Float", u"java/lang/Double",  u"java/lang/Byte",
      u"java/lang/Char",    u"java/lang/Short", u"java/lang/Integer", u"java/lang/Long",
  };

  return klassNames.contains(className) ? Value::from<int32_t>(1) : Value::from<int32_t>(0);
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
  ClassInstance* cls = args[1].get<Instance*>()->asClassInstance();
  Instance* name = args[2].get<Instance*>();

  auto nameStr = utils::getStringValue(name);
  auto field = cls->target()->lookupFieldByName(nameStr);

  if (!field.has_value()) {
    return Value::from<int64_t>(-1);
  }

  return Value::from<int64_t>((*field)->offset());
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

std::optional<Value> jdk_internal_util_SystemProps_Raw_platformProperties(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto stringArrayCls = thread.resolveClass(u"[Ljava/lang/String;");

  auto rawPropsCls = *thread.resolveClass(u"jdk/internal/util/SystemProps$Raw");
  auto arrayLength = rawPropsCls->getStaticFieldValue<int32_t>(u"FIXED_LENGTH", u"I");

  namespace fs = std::filesystem;

  auto temp = fs::temp_directory_path().u16string();

  ArrayInstance* propsArray = thread.heap().allocateArray((*stringArrayCls)->asArrayClass(), arrayLength);
  propsArray->setArrayElement(18, thread.heap().intern(temp));
  propsArray->setArrayElement(36, thread.heap().intern(temp));
  propsArray->setArrayElement(37, thread.heap().intern(temp));
  propsArray->setArrayElement(38, thread.heap().intern(u"user"));
  propsArray->setArrayElement(4, thread.heap().intern(u"UTF-8"));
  propsArray->setArrayElement(19, thread.heap().intern(u"\n"));
  propsArray->setArrayElement(5, thread.heap().intern(u"/"));
  propsArray->setArrayElement(23, thread.heap().intern(u":"));

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

std::optional<Value> java_io_FileOutputStream_writeBytes(JavaThread& thread, CallFrame& frame, const std::vector<Value>& args)
{
  auto self = args.at(0).get<Instance*>();
  auto bytes = args.at(1).get<Instance*>()->asArrayInstance();
  auto offset = args.at(2).get<int32_t>();
  auto length = args.at(3).get<int32_t>();
  auto append = args.at(4).get<int32_t>() == 1;

  auto descriptor = self->getFieldValue<Instance*>(u"fd", u"Ljava/io/FileDescriptor;");
  auto fd = descriptor->getFieldValue<int32_t>(u"fd", u"I");

  std::vector<int8_t> buffer;
  for (int32_t i = 0; i < length; i++) {
    buffer.push_back(bytes->getArrayElement(i)->get<int8_t>());
  }

  FILE* fp = fdopen(fd, "w");
  fwrite(reinterpret_cast<char*>(buffer.data() + offset), sizeof(int8_t), length, fp);

  return std::nullopt;
}

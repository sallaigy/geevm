#include "vm/NativeMethods.h"
#include "vm/Vm.h"

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

static std::optional<Value> noop(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> return_nullptr(Vm& vm, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Object_hashCode(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_System_initProperties(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_System_arraycopy(Vm& vm, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Class_getPrimitiveClass(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_desiredAssertionStatus0(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_getName0(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_forName0(Vm& vm, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_Float_floatToRawIntBits(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Double_doubleToRawIntBits(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Double_longBitsToDouble(Vm& vm, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> sun_misc_Unsafe_arrayBaseOffset(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> sun_reflect_Reflection_getCallerClass(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_security_AccessController_doPrivileged(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Thread_currentThread(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Throwable_fillInStackTrace(Vm& vm, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> java_lang_String_intern(Vm& vm, CallFrame& frame, const std::vector<Value>& args);

static std::optional<Value> geevm_test_print(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  Value value = args[0];

  switch (value.kind()) {
    case Value::Kind::Byte: std::cout << value.asInt() << std::endl; break;
    case Value::Kind::Short: std::cout << value.asInt() << std::endl; break;
    case Value::Kind::Int: {
      PrimitiveType type = *frame.currentMethod()->descriptor().parameters().at(0).asPrimitive();
      if (type == PrimitiveType::Boolean) {
        std::cout << (value.asInt() == 1 ? "true" : "false") << std::endl;
      } else {
        std::cout << value.asInt() << std::endl;
      }
      break;
    }
    case Value::Kind::Long: std::cout << value.asLong() << std::endl; break;
    case Value::Kind::Char: std::cout << value.asInt() << std::endl; break;
    case Value::Kind::Float: std::cout << value.asFloat() << std::endl; break;
    case Value::Kind::Double: std::cout << value.asDouble() << std::endl; break;
    case Value::Kind::ReturnAddress: std::cout << value.asInt() << std::endl; break;
    case Value::Kind::Reference: {
      Instance* ref = value.asReference();
      if (ref->getClass()->className() == u"java/lang/String") {
        types::JString out = u"";
        for (Value charValue : ref->getFieldValue(u"value").asReference()->asArrayInstance()->contents()) {
          out += charValue.asChar();
        }
        std::cout << types::convertJString(out) << std::endl;
      } else {
        std::cout << value.asReference() << std::endl;
      }
      break;
    }
  }

  return std::nullopt;
}

void Vm::registerNatives()
{
  // Temporary printing methods 'org.geevm.tests.basic.Printer'
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/basic/Printer", u"println", u"(I)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/basic/Printer", u"println", u"(J)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/basic/Printer", u"println", u"(F)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/basic/Printer", u"println", u"(D)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/basic/Printer", u"println", u"(Z)V"}, geevm_test_print);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"org/geevm/tests/basic/Printer", u"println", u"(Ljava/lang/String;)V"}, geevm_test_print);

  // java.lang.Object
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"hashCode", u"()I"}, java_lang_Object_hashCode);

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

  // java.lang.String
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/String", u"intern", u"()Ljava/lang/String;"}, java_lang_String_intern);

  // java.lang.Thread
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"currentThread", u"()Ljava/lang/Thread;"}, java_lang_Thread_currentThread);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Thread", u"setPriority0", u"(I)V"}, noop);

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
}

std::optional<Value> noop(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return std::nullopt;
}

std::optional<Value> return_nullptr(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Reference(nullptr);
}

std::optional<Value> java_lang_Object_hashCode(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* objectRef = args[0].asReference();
  return Value::Int(static_cast<int32_t>(std::hash<Instance*>{}(objectRef)));
}

std::optional<Value> java_lang_Class_desiredAssertionStatus0(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Int(0);
}

std::optional<Value> java_lang_Class_getPrimitiveClass(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  auto stringObject = args[0].asReference();
  auto charArray = stringObject->getFieldValue(u"value").asReference()->asArrayInstance();

  types::JString buffer;
  for (Value v : charArray->contents()) {
    buffer += v.asChar();
  }

  if (buffer == u"float") {
    auto klass = vm.resolveClass(u"java/lang/Float");
    return Value::Reference((*klass)->classInstance());
  } else if (buffer == u"double") {
    auto klass = vm.resolveClass(u"java/lang/Double");
    return Value::Reference((*klass)->classInstance());
  } else {
    assert(false && "Unknown primitive class");
  }
}

std::optional<Value> java_lang_Class_getName0(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  JClass* cls = args[0].asReference()->getClass();
  Instance* str = vm.internedStrings().intern(cls->className());

  return Value::Reference(str);
}

std::optional<Value> java_lang_Class_forName0(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* name = args[0].asReference();
  int32_t initialize = args[1].asInt();
  Instance* classLoader = args[2].asReference();

  assert(classLoader == nullptr && "TODO: Support non-boostrap classloader");
  auto charArray = name->getFieldValue(u"value").asReference()->asArrayInstance();

  types::JString nameStr = u"";
  for (Value value : charArray->contents()) {
    nameStr += value.asChar();
  }

  auto loaded = vm.resolveClass(nameStr);
  if (!loaded) {
    // TODO: throw exception
  }
  return Value::Reference((*loaded)->classInstance());
}

std::optional<Value> java_lang_Float_floatToRawIntBits(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Int(std::bit_cast<int32_t>(args[0].asFloat()));
}

std::optional<Value> java_lang_Double_doubleToRawIntBits(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Long(std::bit_cast<int64_t>(args[0].asDouble()));
}

std::optional<Value> java_lang_Double_longBitsToDouble(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Double(std::bit_cast<double>(args[0].asLong()));
}

std::optional<Value> java_lang_System_initProperties(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Reference(args[0].asReference());
}

std::optional<Value> java_lang_System_arraycopy(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
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

std::optional<Value> sun_misc_Unsafe_arrayBaseOffset(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Int(0);
}

std::optional<Value> sun_reflect_Reflection_getCallerClass(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  // Returns the class of the caller of the method calling this method ignoring frames associated with java.lang.reflect.Method.invoke() and its implementation.
  CallFrame* previous = frame.previous()->previous();

  return Value::Reference(previous->currentClass()->classInstance());
}

std::optional<Value> java_security_AccessController_doPrivileged(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  auto actionCls = vm.resolveClass(u"java/security/PrivilegedExceptionAction");
  // TODO: Check loaded

  Instance* target = args[0].asReference();
  auto runMethod = target->getClass()->getMethod(u"run", u"()Ljava/lang/Object;");
  assert(runMethod.has_value());

  frame.pushOperand(args[0]);
  vm.invoke(runMethod->klass, runMethod->method);

  return Value::Reference(frame.popOperand().asReference());
}

std::optional<Value> java_lang_Thread_currentThread(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return Value::Reference(vm.currentThread());
}

std::optional<Value> java_lang_Throwable_fillInStackTrace(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* exceptionInstance = args[0].asReference();

  return Value::Reference(exceptionInstance);
}

std::optional<Value> java_lang_String_intern(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  Instance* self = args.at(0).asReference();
  auto& charArray = self->getFieldValue(u"value").asReference()->asArrayInstance()->contents();

  types::JString str = u"";
  for (auto value : charArray) {
    str += value.asChar();
  }

  Instance* interned = vm.internedStrings().intern(str);

  return Value::Reference(interned);
}

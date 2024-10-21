#include "vm/NativeMethods.h"
#include "vm/Vm.h"

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

static std::optional<Value> java_lang_Object_hashCode(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_System_initProperties(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_getPrimitiveClass(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Class_desiredAssertionStatus0(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Float_floatToRawIntBits(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Double_doubleToRawIntBits(Vm& vm, CallFrame& frame, const std::vector<Value>& args);
static std::optional<Value> java_lang_Double_longBitsToDouble(Vm& vm, CallFrame& frame, const std::vector<Value>& args);

void Vm::registerNatives()
{
  // java.lang.Object
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Object", u"hashCode", u"()I"}, java_lang_Object_hashCode);

  // java.lang.System
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/System", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/System", u"initProperties", u"(Ljava/util/Properties;)Ljava/util/Properties;"},
                                      java_lang_System_initProperties);

  // java.lang.Class
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"registerNatives", u"()V"}, noop);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"getPrimitiveClass", u"(Ljava/lang/String;)Ljava/lang/Class;"},
                                      java_lang_Class_getPrimitiveClass);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Class", u"desiredAssertionStatus0", u"(Ljava/lang/Class;)Z"},
                                      java_lang_Class_desiredAssertionStatus0);

  // java.lang.Float
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Float", u"floatToRawIntBits", u"(F)I"}, java_lang_Float_floatToRawIntBits);

  // java.lang.Double
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Double", u"doubleToRawLongBits", u"(D)J"}, java_lang_Double_doubleToRawIntBits);
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/lang/Double", u"longBitsToDouble", u"(J)D"}, java_lang_Double_longBitsToDouble);

  // sun.misc.VM
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/misc/VM", u"initialize", u"()V"}, noop);

  // sun.misc.Unsafe
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"sun/misc/Unsafe", u"registerNatives", u"()V"}, noop);

  // java.io.FileOutputStream
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/io/FileOutputStream", u"initIDs", u"()V"}, noop);

  // java.io.FileDescriptor
  mNativeMethods.registerNativeMethod(ClassNameAndDescriptor{u"java/io/FileDescriptor", u"initIDs", u"()V"}, noop);
}

std::optional<Value> noop(Vm& vm, CallFrame& frame, const std::vector<Value>& args)
{
  return std::nullopt;
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
#include "vm/NativeMethods.h"
#include "common/DynamicLibrary.h"
#include "common/Encoding.h"
#include "vm/JniImplementation.h"
#include "vm/Thread.h"
#include "vm/Vm.h"
#include "vm/VmUtils.h"

#include <algorithm>
#include <ffi.h>
#include <jni.h>
#include <unordered_set>

using namespace geevm;

static void findReturnType(const MethodDescriptor& descriptor, ffi_type** returnType, jvalue* returnValue, void** result);

std::optional<NativeMethod> NativeMethodRegistry::getNativeMethod(const JMethod* method)
{
  auto dl = DynamicLibrary::create();

  types::JString name = u"Java_";
  types::JString className = method->getClass()->className();
  types::replaceAll(className, u"/", u"_");
  types::replaceAll(className, u"$", u"_00024");

  name += className;
  name += u"_" + method->name();

  auto nameStr = utf16ToUtf8(name);
  void* symbol = dl->findSymbol(nameStr.c_str());

  if (symbol != nullptr) {
    return NativeMethod{method, symbol};
  }

  // Try to find an overloaded method
  auto overloadedMethodName = method->name();
  overloadedMethodName += u"__";

  std::function<types::JString(const FieldType&)> mapParameter = [&mapParameter](const FieldType& parameter) {
    return parameter.map([]<PrimitiveType Type>() {
      return types::JString{PrimitiveTypeTraits<Type>::Descriptor};
    }, [](types::JStringRef className) {
      types::JString buf;
      buf += u"L";
      buf += className;
      types::replaceAll(buf, u"/", u"_");
      buf += u"_2";
      return buf;
    }, [&mapParameter](const ArrayType& arrayType) {
      types::JString buf;
      for (int i = 0; i < arrayType.getDimensions(); i++) {
        buf += u"_1";
      }
      buf += mapParameter(arrayType.getElementType());
      return buf;
    });
  };
  for (const FieldType& parameter : method->descriptor().parameters()) {
    overloadedMethodName += mapParameter(parameter);
  }

  auto overloadedNameStr = utf16ToUtf8(u"Java_" + className + u"_" + overloadedMethodName);
  symbol = dl->findSymbol(overloadedNameStr.c_str());

  if (symbol != nullptr) {
    return NativeMethod{method, symbol};
  }

  return std::nullopt;
}

std::optional<Value> NativeMethod::translateReturnValue(jvalue returnValue) const
{
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
    GEEVM_UNREACHBLE("Unknown primitive type");
  }

  // Must be an instance or array
  return Value::from<Instance*>(jni::translate(returnValue.l).get());
}

void findReturnType(const MethodDescriptor& descriptor, ffi_type** returnType, jvalue* returnValue, void** result)
{
  if (descriptor.returnType().isVoid()) {
    *returnType = &ffi_type_void;
    result = nullptr;
  } else {
    if (auto primitive = descriptor.returnType().getType().asPrimitive(); primitive) {
      switch (*primitive) {
        case PrimitiveType::Byte:
          *returnType = &ffi_type_sint8;
          *result = &returnValue->b;
          break;
        case PrimitiveType::Char:
          *returnType = &ffi_type_uint16;
          *result = &returnValue->c;
          break;
        case PrimitiveType::Double:
          *returnType = &ffi_type_double;
          *result = &returnValue->d;
          break;
        case PrimitiveType::Float:
          *returnType = &ffi_type_float;
          *result = &returnValue->f;
          break;
        case PrimitiveType::Int:
          *returnType = &ffi_type_sint32;
          *result = &returnValue->i;
          break;
        case PrimitiveType::Long:
          *returnType = &ffi_type_sint64;
          *result = &returnValue->j;
          break;
        case PrimitiveType::Short:
          *returnType = &ffi_type_sint16;
          *result = &returnValue->s;
          break;
        case PrimitiveType::Boolean:
          *returnType = &ffi_type_uint8;
          *result = &returnValue->z;
          break;
      }
    } else {
      *returnType = &ffi_type_pointer;
      *result = &returnValue->l;
    }
  }
}

std::optional<Value> NativeMethod::invoke(JavaThread& thread, const std::vector<Value>& args)
{
  // Prepare all state we will need to do the method invocation.
  ffi_cif cif;

  // The types used on the FFI interface
  std::vector<ffi_type*> argTypes;

  // The first parameter is JNIEnv*, a pointer
  argTypes.push_back(&ffi_type_pointer);

  std::vector<jvalue> argValues;
  argValues.reserve(args.size() + 1);

  size_t argIdx = 0;

  JniImplementation impl(thread);
  std::vector<void*> actualArgs;

  JNIEnv* env = impl.getEnv();
  actualArgs.push_back(&env);

  if (mMethod->isStatic()) {
    auto klass = mMethod->getClass()->classInstance();
    argTypes.push_back(&ffi_type_pointer);
    argValues.push_back(jvalue{.l = jni::translate(klass)});
    actualArgs.push_back(&argValues.back().l);
  } else {
    auto javaThis = thread.addJniHandle(args.at(0).get<Instance*>());
    argTypes.push_back(&ffi_type_pointer);
    argValues.push_back(jvalue{.l = jni::translate(javaThis)});
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
          break;
        }
        case PrimitiveType::Long: {
          argValues.push_back(jvalue{.j = current.get<int64_t>()});
          argTypes.push_back(&ffi_type_sint64);
          actualArgs.push_back(&argValues.back().j);
          break;
        }
      }
    } else {
      // Must object or array reference
      GcRootRef<> ref = thread.addJniHandle(current.get<Instance*>());
      argValues.push_back(jvalue{.l = jni::translate(ref)});
      argTypes.push_back(&ffi_type_pointer);
      actualArgs.push_back(&argValues.back().l);
    }
  }

  ffi_type* returnType;
  void* result = nullptr;
  jvalue returnValue;

  findReturnType(mMethod->descriptor(), &returnType, &returnValue, &result);

  assert(actualArgs.size() == mMethod->descriptor().parameters().size() + 2);
  assert(actualArgs.size() == argTypes.size());

  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argTypes.size(), returnType, argTypes.data()) != FFI_OK) {
    thread.throwException(u"java/lang/RuntimeException", u"Failed to invoke native method");
    return std::nullopt;
  }

  ffi_call(&cif, FFI_FN(mHandle), result, actualArgs.data());

  std::optional<Value> toReturn = translateReturnValue(returnValue);

  return toReturn;
}

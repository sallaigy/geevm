#include "vm/Interpreter.h"

#include <iostream>

#include "class_file/Code.h"
#include "class_file/Opcode.h"
#include "vm/Frame.h"
#include "vm/Instance.h"
#include "vm/Vm.h"

#include <cmath>

using namespace geevm;

namespace
{

enum class Predicate
{
  Eq,
  NotEq,
  Lt,
  LtEq,
  Gt,
  GtEq
};

class DefaultInterpreter : public Interpreter
{
public:
  explicit DefaultInterpreter(JavaThread& thread)
    : mThread(thread)
  {
  }

  std::optional<Value> execute(const Code& code, std::size_t startPc) override;

private:
  void invoke(JMethod* method);
  void handleErrorAsException(const VmError& error);

  std::optional<types::u2> tryHandleException(Instance* exception, RuntimeConstantPool& rt, const Code& code, size_t pc);

  CallFrame& currentFrame()
  {
    assert(!mThread.callStack().empty());
    return mThread.currentFrame();
  }

  template<JvmType T>
  void loadAndPush(types::u2 index)
  {
    T val = currentFrame().loadValue<T>(index);
    currentFrame().pushOperand<T>(val);
  }

  template<JvmType T>
  void popAndStore(types::u2 index)
  {
    T val = currentFrame().popOperand<T>();
    currentFrame().storeValue<T>(index, val);
  }

  template<JvmType T>
  void arrayLoad();

  template<JvmType T>
  void arrayStore();

  template<JvmType T>
  void div();

  template<JvmType T>
  void rem();

  template<JvmType T>
  void neg();

  template<JvmType T, class F>
  void simpleOp();

  template<JavaIntegerType T, class ShiftType, uint32_t OffsetMask, bool IsLeft>
  void shift();

  template<JavaFloatType SourceTy, JvmType TargetTy>
  void castFloatToInt();

  template<JavaIntegerType SourceTy, JvmType TargetTy>
  void castInteger();

  template<JvmType T, int32_t NotEqualValue>
  void compare();

  template<JvmType T, class Func>
  void binaryJumpIf(CodeCursor& cursor);

  template<JvmType T, auto CheckedValue, class Func>
  void unaryJumpIf(CodeCursor& cursor);

  void newArray(CodeCursor& cursor);

private:
  JavaThread& mThread;
};

} // namespace

std::unique_ptr<Interpreter> geevm::createDefaultInterpreter(JavaThread& thread)
{
  return std::make_unique<DefaultInterpreter>(thread);
}

static void notImplemented(Opcode opcode)
{
  throw std::runtime_error("Opcode not implemented: " + opcodeToString(opcode));
}

std::optional<Value> DefaultInterpreter::execute(const Code& code, std::size_t startPc)
{
  CodeCursor cursor(code.bytes(), startPc);

  while (cursor.hasNext()) {
    Opcode opcode = cursor.next();
    CallFrame& frame = mThread.currentFrame();
    RuntimeConstantPool& runtimeConstantPool = frame.currentClass()->runtimeConstantPool();

    switch (opcode) {
      using enum Opcode;
      case NOP: notImplemented(opcode); break;
      //==--------------------------------------------------------------------==
      // Constant push
      //==--------------------------------------------------------------------==
      case ACONST_NULL: frame.pushOperand<Instance*>(nullptr); break;
      case ICONST_M1: frame.pushOperand<int32_t>(-1); break;
      case ICONST_0: frame.pushOperand<int32_t>(0); break;
      case ICONST_1: frame.pushOperand<int32_t>(1); break;
      case ICONST_2: frame.pushOperand<int32_t>(2); break;
      case ICONST_3: frame.pushOperand<int32_t>(3); break;
      case ICONST_4: frame.pushOperand<int32_t>(4); break;
      case ICONST_5: frame.pushOperand<int32_t>(5); break;
      case LCONST_0: frame.pushOperand<int64_t>(0); break;
      case LCONST_1: frame.pushOperand<int64_t>(1); break;
      case FCONST_0: frame.pushOperand<float>(0.0f); break;
      case FCONST_1: frame.pushOperand<float>(1.0f); break;
      case FCONST_2: frame.pushOperand<float>(2.0f); break;
      case DCONST_0: frame.pushOperand<double>(0.0); break;
      case DCONST_1: frame.pushOperand<double>(1.0); break;
      case BIPUSH: frame.pushOperand<int32_t>(std::bit_cast<int8_t>(cursor.readU1())); break;
      case SIPUSH: frame.pushOperand<int32_t>(std::bit_cast<int16_t>(cursor.readU2())); break;
      case LDC: {
        types::u1 index = cursor.readU1();
        auto entry = frame.currentClass()->constantPool().getEntry(index);

        if (entry.tag == ConstantPool::Tag::CONSTANT_Integer) {
          frame.pushOperand<int32_t>(entry.data.singleInteger);
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Float) {
          frame.pushOperand<float>(entry.data.singleFloat);
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_String) {
          frame.pushOperand<Instance*>(runtimeConstantPool.getString(index));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Class) {
          auto klass = runtimeConstantPool.getClass(index);
          // TODO: Check if class is loaded
          frame.pushOperand<Instance*>((*klass)->classInstance());
        } else {
          assert(false && "Unknown LDC type!");
        }
        break;
      }
      case LDC_W: {
        types::u2 index = cursor.readU2();
        auto& entry = frame.currentClass()->constantPool().getEntry(index);

        if (entry.tag == ConstantPool::Tag::CONSTANT_Integer) {
          frame.pushOperand<int32_t>(entry.data.singleInteger);
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Float) {
          frame.pushOperand<float>(entry.data.singleFloat);
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_String) {
          frame.pushOperand<Instance*>(runtimeConstantPool.getString(index));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Class) {
          auto klass = runtimeConstantPool.getClass(index);
          // TODO: Check if class is loaded
          frame.pushOperand<Instance*>((*klass)->classInstance());
        } else {
          assert(false && "Unknown LDC_W type!");
        }
        break;
      }
      case LDC2_W: {
        types::u2 index = cursor.readU2();
        auto& entry = frame.currentClass()->constantPool().getEntry(index);

        if (entry.tag == ConstantPool::Tag::CONSTANT_Double) {
          frame.pushOperand<double>(entry.data.doubleFloat);
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Long) {
          frame.pushOperand<int64_t>(entry.data.doubleInteger);
        } else {
          assert(false && "ldc2_w target entry must be double or long");
        }

        break;
      }
      //==--------------------------------------------------------------------==
      // Local variable load and push
      //==--------------------------------------------------------------------==
      case ILOAD: loadAndPush<int32_t>(cursor.readU1()); break;
      case LLOAD: loadAndPush<int64_t>(cursor.readU1()); break;
      case FLOAD: loadAndPush<float>(cursor.readU1()); break;
      case DLOAD: loadAndPush<double>(cursor.readU1()); break;
      case ALOAD: loadAndPush<Instance*>(cursor.readU1()); break;
      case ILOAD_0: loadAndPush<int32_t>(0); break;
      case ILOAD_1: loadAndPush<int32_t>(1); break;
      case ILOAD_2: loadAndPush<int32_t>(2); break;
      case ILOAD_3: loadAndPush<int32_t>(3); break;
      case LLOAD_0: loadAndPush<int64_t>(0); break;
      case LLOAD_1: loadAndPush<int64_t>(1); break;
      case LLOAD_2: loadAndPush<int64_t>(2); break;
      case LLOAD_3: loadAndPush<int64_t>(3); break;
      case FLOAD_0: loadAndPush<float>(0); break;
      case FLOAD_1: loadAndPush<float>(1); break;
      case FLOAD_2: loadAndPush<float>(2); break;
      case FLOAD_3: loadAndPush<float>(3); break;
      case DLOAD_0: loadAndPush<double>(0); break;
      case DLOAD_1: loadAndPush<double>(1); break;
      case DLOAD_2: loadAndPush<double>(2); break;
      case DLOAD_3: loadAndPush<double>(3); break;
      case ALOAD_0: loadAndPush<Instance*>(0); break;
      case ALOAD_1: loadAndPush<Instance*>(1); break;
      case ALOAD_2: loadAndPush<Instance*>(2); break;
      case ALOAD_3: loadAndPush<Instance*>(3); break;
      //==--------------------------------------------------------------------==
      // Array load
      //==--------------------------------------------------------------------==
      case IALOAD: arrayLoad<int32_t>(); break;
      case LALOAD: arrayLoad<int64_t>(); break;
      case FALOAD: arrayLoad<float>(); break;
      case DALOAD: arrayLoad<double>(); break;
      case AALOAD: arrayLoad<Instance*>(); break;
      case BALOAD: arrayLoad<int8_t>(); break;
      case CALOAD: arrayLoad<char16_t>(); break;
      case SALOAD: arrayLoad<int16_t>(); break;
      //==--------------------------------------------------------------------==
      // Local variable store
      //==--------------------------------------------------------------------==
      case ISTORE: popAndStore<int32_t>(cursor.readU1()); break;
      case LSTORE: popAndStore<int64_t>(cursor.readU1()); break;
      case FSTORE: popAndStore<float>(cursor.readU1()); break;
      case DSTORE: popAndStore<double>(cursor.readU1()); break;
      case ASTORE: popAndStore<Instance*>(cursor.readU1()); break;
      case ISTORE_0: popAndStore<int32_t>(0); break;
      case ISTORE_1: popAndStore<int32_t>(1); break;
      case ISTORE_2: popAndStore<int32_t>(2); break;
      case ISTORE_3: popAndStore<int32_t>(3); break;
      case LSTORE_0: popAndStore<int64_t>(0); break;
      case LSTORE_1: popAndStore<int64_t>(1); break;
      case LSTORE_2: popAndStore<int64_t>(2); break;
      case LSTORE_3: popAndStore<int64_t>(3); break;
      case FSTORE_0: popAndStore<float>(0); break;
      case FSTORE_1: popAndStore<float>(1); break;
      case FSTORE_2: popAndStore<float>(2); break;
      case FSTORE_3: popAndStore<float>(3); break;
      case DSTORE_0: popAndStore<double>(0); break;
      case DSTORE_1: popAndStore<double>(1); break;
      case DSTORE_2: popAndStore<double>(2); break;
      case DSTORE_3: popAndStore<double>(3); break;
      case ASTORE_0: popAndStore<Instance*>(0); break;
      case ASTORE_1: popAndStore<Instance*>(1); break;
      case ASTORE_2: popAndStore<Instance*>(2); break;
      case ASTORE_3: popAndStore<Instance*>(3); break;
      //==--------------------------------------------------------------------==
      // Array store
      //==--------------------------------------------------------------------==
      case IASTORE: arrayStore<int32_t>(); break;
      case LASTORE: arrayStore<int64_t>(); break;
      case FASTORE: arrayStore<float>(); break;
      case DASTORE: arrayStore<double>(); break;
      case AASTORE: arrayStore<Instance*>(); break;
      case BASTORE: arrayStore<int8_t>(); break;
      case CASTORE: arrayStore<char16_t>(); break;
      case SASTORE: arrayStore<int16_t>(); break;
      //==--------------------------------------------------------------------==
      // Stack manipulation
      //==--------------------------------------------------------------------==
      case POP: {
        frame.popGenericOperand();
        break;
      }
      case POP2: {
        Value value = frame.popGenericOperand();
        if (!value.isCategoryTwo()) {
          frame.popGenericOperand();
        }

        break;
      }
      case DUP: {
        // TOOD: Duplicate instead of pop / push
        auto value = frame.popGenericOperand();
        frame.pushGenericOperand(value);
        frame.pushGenericOperand(value);
        break;
      }
      case DUP_X1: {
        Value value1 = frame.popGenericOperand();
        Value value2 = frame.popGenericOperand();

        frame.pushGenericOperand(value1);
        frame.pushGenericOperand(value2);
        frame.pushGenericOperand(value1);

        break;
      }
      case DUP_X2: notImplemented(opcode); break;
      case DUP2: {
        // Category 1
        Value value1 = frame.popGenericOperand();
        if (value1.isCategoryTwo()) {
          frame.pushGenericOperand(value1);
          frame.pushGenericOperand(value1);
        } else {
          Value value2 = frame.popGenericOperand();
          frame.pushGenericOperand(value2);
          frame.pushGenericOperand(value1);
          frame.pushGenericOperand(value2);
          frame.pushGenericOperand(value1);
        }

        break;
      }
      case DUP2_X1: notImplemented(opcode); break;
      case DUP2_X2: notImplemented(opcode); break;
      case SWAP: notImplemented(opcode); break;
      //==--------------------------------------------------------------------==
      // Arithmetic operators
      //==--------------------------------------------------------------------==
      case IADD: simpleOp<int32_t, std::plus<int32_t>>(); break;
      case LADD: simpleOp<int64_t, std::plus<int64_t>>(); break;
      case FADD: simpleOp<float, std::plus<float>>(); break;
      case DADD: simpleOp<double, std::plus<double>>(); break;
      case ISUB: simpleOp<int32_t, std::minus<int32_t>>(); break;
      case LSUB: simpleOp<int64_t, std::minus<int64_t>>(); break;
      case FSUB: simpleOp<float, std::minus<float>>(); break;
      case DSUB: simpleOp<double, std::minus<double>>(); break;
      case IMUL: simpleOp<int32_t, std::multiplies<int32_t>>(); break;
      case LMUL: simpleOp<int64_t, std::multiplies<int64_t>>(); break;
      case FMUL: simpleOp<float, std::multiplies<float>>(); break;
      case DMUL: simpleOp<double, std::multiplies<double>>(); break;
      case IDIV: div<int32_t>(); break;
      case LDIV: div<int64_t>(); break;
      case FDIV: div<float>(); break;
      case DDIV: div<double>(); break;
      case IREM: rem<int32_t>(); break;
      case LREM: rem<int64_t>(); break;
      case FREM: rem<float>(); break;
      case DREM: rem<double>(); break;
      case INEG: neg<int32_t>(); break;
      case LNEG: neg<int64_t>(); break;
      case FNEG: neg<float>(); break;
      case DNEG: neg<double>(); break;
      //==--------------------------------------------------------------------==
      // Bit-shift operators
      //==--------------------------------------------------------------------==
      case ISHL: shift<int32_t, int32_t, 0x1F, true>(); break;
      case LSHL: shift<int64_t, int64_t, 0x3F, true>(); break;
      case ISHR: shift<int32_t, int32_t, 0x1F, false>(); break;
      case LSHR: shift<int64_t, int64_t, 0x3F, false>(); break;
      case IUSHR: shift<int32_t, uint32_t, 0x1F, false>(); break;
      case LUSHR: shift<int64_t, uint64_t, 0x3F, false>(); break;
      //==--------------------------------------------------------------------==
      // Bit logic operators
      //==--------------------------------------------------------------------==
      case IAND: simpleOp<int32_t, std::bit_and<int32_t>>(); break;
      case LAND: simpleOp<int64_t, std::bit_and<int64_t>>(); break;
      case IOR: simpleOp<int32_t, std::bit_or<int32_t>>(); break;
      case LOR: simpleOp<int64_t, std::bit_or<int64_t>>(); break;
      case IXOR: simpleOp<int32_t, std::bit_xor<int32_t>>(); break;
      case LXOR: simpleOp<int64_t, std::bit_xor<int64_t>>(); break;
      case IINC: {
        types::u1 index = cursor.readU1();
        auto constValue = static_cast<int32_t>(std::bit_cast<int8_t>(cursor.readU1()));

        frame.storeValue<int32_t>(index, frame.loadValue<int32_t>(index) + constValue);

        break;
      }
      //==--------------------------------------------------------------------==
      // Casts
      //==--------------------------------------------------------------------==
      case I2L: castInteger<int32_t, int64_t>(); break;
      case I2F: castInteger<int32_t, float>(); break;
      case I2D: castInteger<int32_t, double>(); break;
      case L2I: castInteger<int64_t, int32_t>(); break;
      case L2F: castInteger<int64_t, float>(); break;
      case L2D: castInteger<int64_t, double>(); break;
      case F2I: castFloatToInt<float, int32_t>(); break;
      case F2L: castFloatToInt<float, int64_t>(); break;
      case F2D: frame.pushOperand<double>(static_cast<double>(frame.popOperand<float>())); break;
      case D2I: castFloatToInt<double, int32_t>(); break;
      case D2L: castFloatToInt<double, int64_t>(); break;
      case D2F: frame.pushOperand<float>(static_cast<float>(frame.popOperand<double>())); break;
      case I2B: castInteger<int32_t, int8_t>(); break;
      case I2C: castInteger<int32_t, char16_t>(); break;
      case I2S: castInteger<int32_t, int16_t>(); break;
      //==--------------------------------------------------------------------==
      // Comparisons
      //==--------------------------------------------------------------------==
      case LCMP: compare<int64_t, 1>(); break;
      case FCMPL: compare<float, -1>(); break;
      case FCMPG: compare<float, 1>(); break;
      case DCMPL: compare<double, -1>(); break;
      case DCMPG: compare<double, 1>(); break;
      case IFEQ: unaryJumpIf<int32_t, 0, std::equal_to<int32_t>>(cursor); break;
      case IFNE: unaryJumpIf<int32_t, 0, std::not_equal_to<int32_t>>(cursor); break;
      case IFLT: unaryJumpIf<int32_t, 0, std::less<int32_t>>(cursor); break;
      case IFGE: unaryJumpIf<int32_t, 0, std::greater_equal<int32_t>>(cursor); break;
      case IFGT: unaryJumpIf<int32_t, 0, std::greater<int32_t>>(cursor); break;
      case IFLE: unaryJumpIf<int32_t, 0, std::less_equal<int32_t>>(cursor); break;
      //==--------------------------------------------------------------------==
      // Jumps
      //==--------------------------------------------------------------------==
      case IF_ICMPEQ: binaryJumpIf<int32_t, std::equal_to<int32_t>>(cursor); break;
      case IF_ICMPNE: binaryJumpIf<int32_t, std::not_equal_to<int32_t>>(cursor); break;
      case IF_ICMPLT: binaryJumpIf<int32_t, std::less<int32_t>>(cursor); break;
      case IF_ICMPGE: binaryJumpIf<int32_t, std::greater_equal<int32_t>>(cursor); break;
      case IF_ICMPGT: binaryJumpIf<int32_t, std::greater<int32_t>>(cursor); break;
      case IF_ICMPLE: binaryJumpIf<int32_t, std::less_equal<int32_t>>(cursor); break;
      case IF_ACMPEQ: binaryJumpIf<Instance*, std::equal_to<Instance*>>(cursor); break;
      case IF_ACMPNE: binaryJumpIf<Instance*, std::not_equal_to<Instance*>>(cursor); break;
      case GOTO: {
        auto opcodePos = cursor.position() - 1;
        auto offset = std::bit_cast<int16_t>(cursor.readU2());

        cursor.set(opcodePos + offset);
        break;
      }
      case JSR: notImplemented(opcode); break;
      case RET: notImplemented(opcode); break;
      case TABLESWITCH: notImplemented(opcode); break;
      case LOOKUPSWITCH: notImplemented(opcode); break;
      //==--------------------------------------------------------------------==
      // Returns
      //==--------------------------------------------------------------------==
      case IRETURN: return Value::from<int32_t>(frame.popOperand<int32_t>());
      case LRETURN: return Value::from<int64_t>(frame.popOperand<int64_t>());
      case FRETURN: return Value::from<float>(frame.popOperand<float>());
      case DRETURN: return Value::from<double>(frame.popOperand<double>());
      case ARETURN: return Value::from<Instance*>(frame.popOperand<Instance*>());
      case RETURN: return std::nullopt;
      //==--------------------------------------------------------------------==
      // Field manipulation
      //==--------------------------------------------------------------------==
      case GETSTATIC: {
        auto index = cursor.readU2();
        const JField* field = runtimeConstantPool.getFieldRef(index);

        JClass* klass = field->getClass();
        klass->initialize(mThread);

        Value value = klass->getStaticFieldValue(field->offset());
        frame.pushGenericOperand(value);

        break;
      }
      case PUTSTATIC: {
        auto index = cursor.readU2();
        const JField* field = runtimeConstantPool.getFieldRef(index);

        JClass* klass = field->getClass();
        klass->initialize(mThread);

        klass->setStaticFieldValue(field->offset(), frame.popGenericOperand());

        break;
      }
      case GETFIELD: {
        types::u2 index = cursor.readU2();
        const JField* field = runtimeConstantPool.getFieldRef(index);
        auto objectRef = frame.popOperand<Instance*>();

        if (objectRef == nullptr) {
          mThread.throwException(u"java/lang/NullPointerException");
          break;
        }

        assert(objectRef->getClass()->isInstanceOf(field->getClass()));
        frame.pushGenericOperand(objectRef->getFieldValue(field->name(), field->descriptor()));
        break;
      }
      case PUTFIELD: {
        auto index = cursor.readU2();
        auto field = runtimeConstantPool.getFieldRef(index);

        Value value = frame.popGenericOperand();
        auto objectRef = frame.popOperand<Instance*>();

        if (objectRef == nullptr) {
          mThread.throwException(u"java/lang/NullPointerException");
          break;
        }

        assert(objectRef->getClass()->isInstanceOf(field->getClass()));
        objectRef->setFieldValue(field->name(), field->descriptor(), value);

        break;
      }
      //==--------------------------------------------------------------------==
      // Invocations
      //==--------------------------------------------------------------------==
      case INVOKEVIRTUAL: {
        auto index = cursor.readU2();
        const JMethod* baseMethod = runtimeConstantPool.getMethodRef(index);

        int numArgs = baseMethod->descriptor().parameters().size();
        auto objectRef = frame.peek(numArgs).get<Instance*>();
        if (objectRef == nullptr) {
          mThread.throwException(u"java/lang/NullPointerException");
          break;
        }

        JClass* target = objectRef->getClass();
        auto targetMethod = target->getVirtualMethod(baseMethod->name(), baseMethod->rawDescriptor());

        assert(targetMethod.has_value());

        this->invoke(*targetMethod);

        break;
      }
      case INVOKESPECIAL: {
        auto index = cursor.readU2();
        JMethod* method = runtimeConstantPool.getMethodRef(index);

        this->invoke(method);

        break;
      }
      case INVOKESTATIC: {
        auto index = cursor.readU2();
        JMethod* method = runtimeConstantPool.getMethodRef(index);
        assert(method->isStatic());

        method->getClass()->initialize(mThread);

        this->invoke(method);

        break;
      }
      case INVOKEINTERFACE: {
        auto index = cursor.readU2();
        const JMethod* methodRef = runtimeConstantPool.getMethodRef(index);

        // Consume 'count'
        cursor.readU1();
        // Consume '0'
        cursor.readU1();

        int numArgs = methodRef->descriptor().parameters().size();
        Value objectRef = frame.peek(numArgs);

        JClass* target = objectRef.get<Instance*>()->getClass();
        auto method = target->getVirtualMethod(methodRef->name(), methodRef->rawDescriptor());

        this->invoke(*method);
        break;
      }
      case INVOKEDYNAMIC: notImplemented(opcode); break;
      //==--------------------------------------------------------------------==
      // OOP
      //==--------------------------------------------------------------------==
      case NEW: {
        auto index = cursor.readU2();
        auto className = frame.currentClass()->constantPool().getClassName(index);

        auto klass = mThread.resolveClass(types::JString{className});
        if (!klass) {
          this->handleErrorAsException(klass.error());
          break;
        }

        (*klass)->initialize(mThread);

        if (auto instanceClass = (*klass)->asInstanceClass(); instanceClass != nullptr) {
          Instance* instance = mThread.heap().allocate(instanceClass);
          frame.pushOperand<Instance*>(instance);
        } else {
          assert(false && "TODO new with array class");
        }

        break;
      }
      case NEWARRAY: newArray(cursor); break;
      case ANEWARRAY: {
        auto index = cursor.readU2();
        int32_t count = frame.popOperand<int32_t>();

        auto klass = runtimeConstantPool.getClass(index);
        if (!klass) {
          this->handleErrorAsException(klass.error());
          break;
        }

        auto arrayClass = mThread.resolveClass(u"[L" + types::JString{(*klass)->className()} + u";");
        if (!arrayClass) {
          this->handleErrorAsException(arrayClass.error());
          break;
        }

        if (count < 0) {
          mThread.throwException(u"java/lang/NegativeArraySizeException", u"");
          break;
        }

        ArrayInstance* array = mThread.heap().allocateArray((*arrayClass)->asArrayClass(), count);
        frame.pushOperand<Instance*>(array);

        break;
      }
      case ARRAYLENGTH: {
        ArrayInstance* arrayRef = frame.popOperand<Instance*>()->asArrayInstance();
        frame.pushOperand<int32_t>(arrayRef->length());
        break;
      }
      case ATHROW: {
        auto exception = frame.popOperand<Instance*>();
        mThread.throwException(exception);
        break;
      }
      case CHECKCAST: {
        types::u2 index = cursor.readU2();
        auto objectRef = frame.popOperand<Instance*>();
        if (objectRef == nullptr) {
          frame.pushOperand<Instance*>(objectRef);
        } else {
          auto klass = runtimeConstantPool.getClass(index);
          if (!klass) {
            this->handleErrorAsException(klass.error());
            break;
          }

          JClass* classToCheck = objectRef->getClass();
          if (!classToCheck->isInstanceOf(*klass)) {
            types::JString message = u"class " + classToCheck->javaClassName() + u" cannot be cast to class " + (*klass)->javaClassName();
            mThread.throwException(u"java/lang/ClassCastException", message);
          } else {
            frame.pushOperand<Instance*>(objectRef);
          }
        }

        break;
      }
      case INSTANCEOF: {
        auto index = cursor.readU2();
        Instance* objectRef = frame.popOperand<Instance*>();
        if (objectRef == nullptr) {
          frame.pushOperand<int32_t>(0);
        } else {
          auto klass = runtimeConstantPool.getClass(index);
          if (!klass) {
            this->handleErrorAsException(klass.error());
            break;
          }

          JClass* classToCheck = objectRef->getClass();
          if (classToCheck->isInstanceOf(*klass)) {
            frame.pushOperand<int32_t>(1);
          } else {
            frame.pushOperand<int32_t>(0);
          }
        }

        break;
      }
      case MONITORENTER: {
        // FIXME
        frame.popOperand<Instance*>();
        break;
      }
      case MONITOREXIT: {
        // FIXME
        frame.popOperand<Instance*>();
        break;
      }

      case WIDE: notImplemented(opcode); break;
      case MULTIANEWARRAY: {
        uint16_t index = cursor.readU2();
        uint8_t dimensions = cursor.readU1();

        auto klass = runtimeConstantPool.getClass(index);
        if (!klass) {
          this->handleErrorAsException(klass.error());
          break;
        }

        std::vector<int32_t> dimensionCounts;
        for (uint8_t dim = 0; dim < dimensions; dim++) {
          dimensionCounts.push_back(frame.popOperand<int32_t>());
        }

        std::optional<JClass*> elementClass = (*klass)->asArrayClass()->elementClass();
        assert(elementClass.has_value());

        auto makeInnerArray = [this](auto& self, std::vector<int32_t> dimensionCounts, ArrayClass* arrayClass) -> ArrayInstance* {
          auto count = dimensionCounts.back();
          dimensionCounts.pop_back();

          ArrayInstance* newArray = mThread.heap().allocateArray(arrayClass, count);
          if (!dimensionCounts.empty()) {
            ArrayClass* innerArrayClass = (*arrayClass->elementClass())->asArrayClass();
            for (int32_t i = 0; i < count; i++) {
              auto innerArray = self(self, dimensionCounts, innerArrayClass);
              newArray->setArrayElement(i, Value::from<Instance*>(innerArray));
            }
          }

          return newArray;
        };

        ArrayInstance* result = makeInnerArray(makeInnerArray, dimensionCounts, (*klass)->asArrayClass());

        frame.pushOperand<Instance*>(result);
        break;
      }
      case IFNULL: unaryJumpIf<Instance*, nullptr, std::equal_to<Instance*>>(cursor); break;
      case IFNONNULL: unaryJumpIf<Instance*, nullptr, std::not_equal_to<Instance*>>(cursor); break;
      case GOTO_W: notImplemented(opcode); break;
      case JSR_W: notImplemented(opcode); break;
      case BREAKPOINT:
      case IMPDEP1:
      case IMPDEP2:
        // Reserved opcodes
        break;
      default: assert(false && "Unknown opcode!");
    }

    // Handle exception
    if (auto exception = mThread.currentException(); exception != nullptr) {
      size_t pc = cursor.position() - 1;

      auto newPc = this->tryHandleException(exception, runtimeConstantPool, code, pc);

      if (newPc.has_value()) {
        cursor.set(newPc.value());
        mThread.clearException();
      } else {
        return std::nullopt;
      }
    }
  }

  assert(false && "Should be unreachable");
}

std::optional<types::u2> DefaultInterpreter::tryHandleException(Instance* exception, RuntimeConstantPool& rt, const Code& code, size_t pc)
{
  for (auto& entry : code.exceptionTable()) {
    if (pc < entry.startPc || pc >= entry.endPc) {
      // The exception handler is not active
      continue;
    }

    bool caught = false;
    if (entry.catchType != 0) {
      auto exceptionClass = rt.getClass(entry.catchType);

      assert(exceptionClass.has_value());
      if (exception->getClass()->isInstanceOf(*exceptionClass)) {
        caught = true;
      }
    } else {
      caught = true;
    }

    if (caught) {
      return entry.handlerPc;
    }
  }

  return std::nullopt;
}

static bool compareInt(Predicate predicate, int32_t val1, int32_t val2)
{
  switch (predicate) {
    using enum Predicate;
    case Eq: return val1 == val2;
    case NotEq: return val1 != val2;
    case Gt: return val1 > val2;
    case Lt: return val1 < val2;
    case GtEq: return val1 >= val2;
    case LtEq: return val1 <= val2;
  }

  std::unreachable();
}

void DefaultInterpreter::invoke(JMethod* method)
{
  auto returnValue = mThread.invoke(method);
  assert((method->isVoid() || mThread.currentException() != nullptr) || returnValue.has_value());

  if (returnValue.has_value()) {
    mThread.currentFrame().pushGenericOperand(*returnValue);
  }
}

void DefaultInterpreter::handleErrorAsException(const VmError& error)
{
  mThread.throwException(error.exception(), error.message());
}

template<JvmType T>
void DefaultInterpreter::arrayLoad()
{
  auto index = currentFrame().popOperand<int32_t>();
  auto arrayRef = currentFrame().popOperand<Instance*>();

  if (arrayRef == nullptr) {
    mThread.throwException(u"java/lang/NullPointerException");
    return;
  }

  ArrayInstance* array = arrayRef->asArrayInstance();
  auto element = array->getArrayElement<T>(index);
  if (!element) {
    this->handleErrorAsException(element.error());
    return;
  }

  if constexpr (StoredAsInt<T>) {
    currentFrame().pushOperand<int32_t>(static_cast<int32_t>(*element));
  } else {
    currentFrame().pushOperand<T>(*element);
  }
}

template<JvmType T>
void DefaultInterpreter::arrayStore()
{
  T value;
  if constexpr (StoredAsInt<T>) {
    value = static_cast<T>(currentFrame().popOperand<int32_t>());
  } else {
    value = currentFrame().popOperand<T>();
  }

  auto index = currentFrame().popOperand<int32_t>();
  auto arrayRef = currentFrame().popOperand<Instance*>();

  if (arrayRef == nullptr) {
    mThread.throwException(u"java/lang/NullPointerException");
    return;
  }

  ArrayInstance* array = arrayRef->asArrayInstance();

  if constexpr (std::is_same_v<std::remove_const_t<T>, Instance*>) {
    if (value != nullptr) {
      const JClass* elementClass = value->getClass();
      auto arrayElementClass = array->getClass()->asArrayClass()->elementClass();
      assert(arrayElementClass);

      if (!elementClass->isInstanceOf(*arrayElementClass)) {
        mThread.throwException(u"java/lang/ArrayStoreException");
      }
    }
  }

  JvmExpected<void> result = array->setArrayElement<T>(index, value);
  if (!result) {
    this->handleErrorAsException(result.error());
  }
}

template<JvmType T>
void DefaultInterpreter::div()
{
  T value2 = currentFrame().popOperand<T>();
  T value1 = currentFrame().popOperand<T>();

  if constexpr (JavaIntegerType<T>) {
    if (value2 == 0) {
      mThread.throwException(u"java/lang/ArithmeticException", u"Divison by zero");
    }
  }

  T result = value1 / value2;
  currentFrame().pushOperand<T>(result);
}

template<JvmType T>
void DefaultInterpreter::rem()
{
  T value2 = currentFrame().popOperand<T>();
  T value1 = currentFrame().popOperand<T>();

  if constexpr (JavaIntegerType<T>) {
    T result = value1 - (value1 / value2) * value2;
    currentFrame().pushOperand<T>(result);
  } else {
    T result = std::fmod(value1, value2);
    currentFrame().pushOperand<T>(result);
  }
}

template<JvmType T>
void DefaultInterpreter::neg()
{
  T value = currentFrame().popOperand<T>();
  T result = -value;
  currentFrame().pushOperand<T>(result);
}

template<JvmType T, class F>
void DefaultInterpreter::simpleOp()
{
  T value2 = currentFrame().popOperand<T>();
  T value1 = currentFrame().popOperand<T>();

  T result = F{}(value1, value2);
  currentFrame().pushOperand<T>(result);
}

template<JavaIntegerType T, class ShiftType, uint32_t OffsetMask, bool IsLeft>
void DefaultInterpreter::shift()
{
  auto value2 = currentFrame().popOperand<int32_t>();
  auto value1 = currentFrame().popOperand<T>();

  auto target = std::bit_cast<ShiftType>(value1);
  auto offset = std::bit_cast<uint32_t>(value2) & OffsetMask;

  T result;
  if constexpr (IsLeft) {
    result = static_cast<T>(target << offset);
  } else {
    result = static_cast<T>(target >> offset);
  }

  currentFrame().pushOperand<T>(result);
}

template<JvmType T, class Func>
void DefaultInterpreter::binaryJumpIf(CodeCursor& cursor)
{
  auto opcodePos = cursor.position() - 1;

  auto val2 = currentFrame().popOperand<T>();
  auto val1 = currentFrame().popOperand<T>();

  auto offset = std::bit_cast<int16_t>(cursor.readU2());

  if (Func{}(val1, val2)) {
    cursor.set(opcodePos + offset);
  }
}

template<JvmType T, auto CheckedValue, class Func>
void DefaultInterpreter::unaryJumpIf(CodeCursor& cursor)
{
  auto opcodePos = cursor.position() - 1;
  auto value = currentFrame().popOperand<T>();

  auto offset = std::bit_cast<int16_t>(cursor.readU2());

  if (Func{}(value, static_cast<T>(CheckedValue))) {
    cursor.set(opcodePos + offset);
  }
}

template<JavaFloatType SourceTy, JvmType TargetTy>
void DefaultInterpreter::castFloatToInt()
{
  auto value = currentFrame().popOperand<SourceTy>();

  TargetTy result;
  if (std::isnan(value)) {
    result = 0;
  } else {
    auto truncated = value > 0 ? std::floor(value) : std::ceil(value);
    if (static_cast<SourceTy>(std::numeric_limits<TargetTy>::max()) > truncated && static_cast<SourceTy>(std::numeric_limits<TargetTy>::min()) < truncated) {
      result = static_cast<TargetTy>(truncated);
    } else if (truncated < 0) {
      result = std::numeric_limits<TargetTy>::min();
    } else {
      result = std::numeric_limits<TargetTy>::max();
    }
  }

  currentFrame().pushOperand<TargetTy>(result);
}

template<JavaIntegerType SourceTy, JvmType TargetTy>
void DefaultInterpreter::castInteger()
{
  auto value = currentFrame().popOperand<SourceTy>();

  if constexpr (StoredAsInt<TargetTy>) {
    currentFrame().pushOperand<int32_t>(static_cast<TargetTy>(value));
  } else {
    currentFrame().pushOperand<TargetTy>(static_cast<TargetTy>(value));
  }
}

template<JvmType T, int32_t NotEqualValue>
void DefaultInterpreter::compare()
{
  T value2 = currentFrame().popOperand<T>();
  T value1 = currentFrame().popOperand<T>();

  if (value1 > value2) {
    currentFrame().pushOperand<int32_t>(1);
  } else if (value1 == value2) {
    currentFrame().pushOperand<int32_t>(0);
  } else if (value1 < value2) {
    currentFrame().pushOperand<int32_t>(-1);
  } else {
    // This is only possible for floating-point comparisons (NaN comparisons are always false).
    currentFrame().pushOperand<int32_t>(NotEqualValue);
  }
}

void DefaultInterpreter::newArray(CodeCursor& cursor)
{
  enum class ArrayType
  {
    T_BOOLEAN = 4,
    T_CHAR = 5,
    T_FLOAT = 6,
    T_DOUBLE = 7,
    T_BYTE = 8,
    T_SHORT = 9,
    T_INT = 10,
    T_LONG = 11,
  };

  auto arrayType = static_cast<ArrayType>(cursor.readU1());
  auto count = currentFrame().popOperand<int32_t>();

  types::JString arrayClsName;

  switch (arrayType) {
    case ArrayType::T_BOOLEAN: arrayClsName = u"[Z"; break;
    case ArrayType::T_CHAR: arrayClsName = u"[C"; break;
    case ArrayType::T_FLOAT: arrayClsName = u"[F"; break;
    case ArrayType::T_DOUBLE: arrayClsName = u"[D"; break;
    case ArrayType::T_BYTE: arrayClsName = u"[B"; break;
    case ArrayType::T_SHORT: arrayClsName = u"[S"; break;
    case ArrayType::T_INT: arrayClsName = u"[I"; break;
    case ArrayType::T_LONG: arrayClsName = u"[J"; break;
    default: assert(false && "impossible"); break;
  }

  auto arrayClass = mThread.resolveClass(arrayClsName);
  if (!arrayClass) {
    this->handleErrorAsException(arrayClass.error());
    return;
  }

  if (count < 0) {
    mThread.throwException(u"java/lang/NegativeArraySizeException", u"");
    return;
  }

  ArrayInstance* newInstance = mThread.heap().allocateArray((*arrayClass)->asArrayClass(), count);
  currentFrame().pushOperand<Instance*>(newInstance);
}

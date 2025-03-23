#include "class_file/Opcode.h"
#include "jit/JitCompiler.h"
#include "vm/Frame.h"
#include "vm/Instance.h"
#include "vm/Vm.h"

#include <cmath>
#include <format>

using namespace geevm;

namespace
{

class DefaultInterpreter
{
public:
  explicit DefaultInterpreter(JavaThread& thread)
    : mThread(thread), mCurrentFrame(&thread.currentFrame())
  {
  }

  DefaultInterpreter(const DefaultInterpreter&) = delete;
  DefaultInterpreter& operator=(const DefaultInterpreter&) = delete;

  std::optional<Value> execute();

private:
  void invoke(JMethod* method);
  void handleErrorAsException(const VmError& error);

  std::optional<types::u2> tryHandleException(GcRootRef<> exception, const Code& code, size_t pc);

  CallFrame& currentFrame()
  {
    assert(mCurrentFrame != nullptr);
    return *mCurrentFrame;
  }

  template<JvmType T>
  void loadAndPush(size_t index)
  {
    T val = currentFrame().loadValue<T>(index);
    currentFrame().pushOperand<T>(val);
  }

  template<JvmType T>
  void popAndStore(size_t index)
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

  template<CategoryOneJvmType T, class F>
  void simpleOp();

  template<CategoryTwoJvmType T, class F>
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
  void binaryJumpIf();

  template<JvmType T, auto CheckedValue, class Func>
  void unaryJumpIf();

  void newObject();
  void newArray();
  void newReferenceArray();
  void newMultiArray();

  void checkCast();
  void instanceOf();

  template<std::signed_integral T, class F>
  struct WrapSignedArithmetic
  {
    T operator()(T a, T b) const
    {
      using U = std::make_unsigned_t<T>;
      auto result = F{}(std::bit_cast<U>(a), std::bit_cast<U>(b));

      return std::bit_cast<T>(result);
    }
  };

  void invokeVirtual();
  void invokeStatic();
  void invokeSpecial();
  void invokeInterface();

  void getStatic();
  void putStatic();
  void getField();
  void putField();

  void swap();
  void dup();
  void dupX1();
  void dupX2();
  void dup2();
  void dup2X1();
  void dup2X2();

  void ldc(types::u2 index);
  void ldc2_w(types::u2 index);
  void lookupSwitch();
  void tableSwitch();
  void wide(Opcode modifiedOpcode);

  bool checkException();

private:
  JavaThread& mThread;
  CallFrame* mCurrentFrame = nullptr;
};

} // namespace

std::optional<Value> JavaThread::executeTopFrame()
{
  JMethod* method = this->currentFrame().currentMethod();
  if (method->jitFunction() != nullptr) {
    // FIXME: Void returns
    uint64_t result = method->jitFunction()(this->currentFrame().locals());
    return Value{result};
  }

  if (!mVm.settings().jitFunctions.empty()) {
    std::string signature = method->signatureString();
    if (mVm.settings().jitFunctions.contains(signature)) {
      JitFunction fn = mVm.jit().compile(this->currentFrame().currentMethod());
      if (fn != nullptr) {
        // FIXME: Void returns
        uint64_t result = fn(this->currentFrame().locals());
        method->setJitFunction(fn);

        return Value{result};
      } else {
        geevm_panic("Failed to JIT-compile");
      }
    }
  }

  DefaultInterpreter interpreter{*this};
  return interpreter.execute();
}

static void notImplemented(Opcode opcode)
{
  geevm_panic(std::format("using unsupported opcode '{}'", opcodeToString(opcode)));
}

bool DefaultInterpreter::checkException()
{
  const Code& code = mCurrentFrame->currentMethod()->getCode();
  auto exception = mThread.currentException();
  assert(exception != nullptr);
  size_t pc = mCurrentFrame->programCounter() - 1;

  auto newPc = this->tryHandleException(exception, code, pc);

  if (newPc.has_value()) {
    mCurrentFrame->set(newPc.value());
    mThread.clearException();
  } else {
    return true;
  }

  return false;
}

#define WITH_EXCEPTION_CHECK(INSTRUCTION)                     \
  {                                                           \
    INSTRUCTION;                                              \
    if (mThread.currentException() != nullptr) [[unlikely]] { \
      bool uncaughtException = this->checkException();        \
      if (uncaughtException) {                                \
        return std::nullopt;                                  \
      }                                                       \
    }                                                         \
  }

std::optional<Value> DefaultInterpreter::execute()
{
  while (true) {
    Opcode opcode = mCurrentFrame->next();

    switch (opcode) {
      using enum Opcode;
      //==--------------------------------------------------------------------==
      // Constant push
      //==--------------------------------------------------------------------==
      case ACONST_NULL: mCurrentFrame->pushOperand<Instance*>(nullptr); break;
      case ICONST_M1: mCurrentFrame->pushOperand<int32_t>(-1); break;
      case ICONST_0: mCurrentFrame->pushOperand<int32_t>(0); break;
      case ICONST_1: mCurrentFrame->pushOperand<int32_t>(1); break;
      case ICONST_2: mCurrentFrame->pushOperand<int32_t>(2); break;
      case ICONST_3: mCurrentFrame->pushOperand<int32_t>(3); break;
      case ICONST_4: mCurrentFrame->pushOperand<int32_t>(4); break;
      case ICONST_5: mCurrentFrame->pushOperand<int32_t>(5); break;
      case LCONST_0: mCurrentFrame->pushOperand<int64_t>(0); break;
      case LCONST_1: mCurrentFrame->pushOperand<int64_t>(1); break;
      case FCONST_0: mCurrentFrame->pushOperand<float>(0.0f); break;
      case FCONST_1: mCurrentFrame->pushOperand<float>(1.0f); break;
      case FCONST_2: mCurrentFrame->pushOperand<float>(2.0f); break;
      case DCONST_0: mCurrentFrame->pushOperand<double>(0.0); break;
      case DCONST_1: mCurrentFrame->pushOperand<double>(1.0); break;
      case BIPUSH: mCurrentFrame->pushOperand<int32_t>(std::bit_cast<int8_t>(mCurrentFrame->readU1())); break;
      case SIPUSH: mCurrentFrame->pushOperand<int32_t>(std::bit_cast<int16_t>(mCurrentFrame->readU2())); break;
      case LDC: WITH_EXCEPTION_CHECK(ldc(mCurrentFrame->readU1())); break;
      case LDC_W: WITH_EXCEPTION_CHECK(ldc(mCurrentFrame->readU2())); break;
      case LDC2_W: WITH_EXCEPTION_CHECK(ldc2_w(mCurrentFrame->readU2())); break;
      //==--------------------------------------------------------------------==
      // Local variable load and push
      //==--------------------------------------------------------------------==
      case ILOAD: loadAndPush<int32_t>(mCurrentFrame->readU1()); break;
      case LLOAD: loadAndPush<int64_t>(mCurrentFrame->readU1()); break;
      case FLOAD: loadAndPush<float>(mCurrentFrame->readU1()); break;
      case DLOAD: loadAndPush<double>(mCurrentFrame->readU1()); break;
      case ALOAD: loadAndPush<Instance*>(mCurrentFrame->readU1()); break;
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
      case IALOAD: WITH_EXCEPTION_CHECK(arrayLoad<int32_t>()) break;
      case LALOAD: WITH_EXCEPTION_CHECK(arrayLoad<int64_t>()) break;
      case FALOAD: WITH_EXCEPTION_CHECK(arrayLoad<float>()); break;
      case DALOAD: WITH_EXCEPTION_CHECK(arrayLoad<double>()); break;
      case AALOAD: WITH_EXCEPTION_CHECK(arrayLoad<Instance*>()); break;
      case BALOAD: WITH_EXCEPTION_CHECK(arrayLoad<int8_t>()); break;
      case CALOAD: WITH_EXCEPTION_CHECK(arrayLoad<char16_t>()); break;
      case SALOAD: WITH_EXCEPTION_CHECK(arrayLoad<int16_t>()); break;
      //==--------------------------------------------------------------------==
      // Local variable store
      //==--------------------------------------------------------------------==
      case ISTORE: popAndStore<int32_t>(mCurrentFrame->readU1()); break;
      case LSTORE: popAndStore<int64_t>(mCurrentFrame->readU1()); break;
      case FSTORE: popAndStore<float>(mCurrentFrame->readU1()); break;
      case DSTORE: popAndStore<double>(mCurrentFrame->readU1()); break;
      case ASTORE: popAndStore<Instance*>(mCurrentFrame->readU1()); break;
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
      case IASTORE: WITH_EXCEPTION_CHECK(arrayStore<int32_t>()); break;
      case LASTORE: WITH_EXCEPTION_CHECK(arrayStore<int64_t>()); break;
      case FASTORE: WITH_EXCEPTION_CHECK(arrayStore<float>()); break;
      case DASTORE: WITH_EXCEPTION_CHECK(arrayStore<double>()); break;
      case AASTORE: WITH_EXCEPTION_CHECK(arrayStore<Instance*>()); break;
      case BASTORE: WITH_EXCEPTION_CHECK(arrayStore<int8_t>()); break;
      case CASTORE: WITH_EXCEPTION_CHECK(arrayStore<char16_t>()); break;
      case SASTORE: WITH_EXCEPTION_CHECK(arrayStore<int16_t>()); break;
      //==--------------------------------------------------------------------==
      // Stack manipulation
      //==--------------------------------------------------------------------==
      case POP: {
        mCurrentFrame->popGenericOperand();
        break;
      }
      case POP2: {
        mCurrentFrame->popGenericOperand();
        mCurrentFrame->popGenericOperand();
        break;
      }
      case DUP: dup(); break;
      case DUP_X1: dupX1(); break;
      case DUP_X2: dupX2(); break;
      case DUP2: dup2(); break;
      case DUP2_X1: dup2X1(); break;
      case DUP2_X2: dup2X2(); break;
      case SWAP: swap(); break;
      //==--------------------------------------------------------------------==
      // Arithmetic operators
      //==--------------------------------------------------------------------==
      case IADD: simpleOp<int32_t, WrapSignedArithmetic<int32_t, std::plus<>>>(); break;
      case LADD: simpleOp<int64_t, WrapSignedArithmetic<int64_t, std::plus<>>>(); break;
      case FADD: simpleOp<float, std::plus<float>>(); break;
      case DADD: simpleOp<double, std::plus<double>>(); break;
      case ISUB: simpleOp<int32_t, WrapSignedArithmetic<int32_t, std::minus<>>>(); break;
      case LSUB: simpleOp<int64_t, WrapSignedArithmetic<int64_t, std::minus<>>>(); break;
      case FSUB: simpleOp<float, std::minus<float>>(); break;
      case DSUB: simpleOp<double, std::minus<double>>(); break;
      case IMUL: simpleOp<int32_t, WrapSignedArithmetic<int32_t, std::multiplies<>>>(); break;
      case LMUL: simpleOp<int64_t, WrapSignedArithmetic<int64_t, std::multiplies<>>>(); break;
      case FMUL: simpleOp<float, std::multiplies<float>>(); break;
      case DMUL: simpleOp<double, std::multiplies<double>>(); break;
      case IDIV: WITH_EXCEPTION_CHECK(div<int32_t>()); break;
      case LDIV: WITH_EXCEPTION_CHECK(div<int64_t>()); break;
      case FDIV: WITH_EXCEPTION_CHECK(div<float>()); break;
      case DDIV: WITH_EXCEPTION_CHECK(div<double>()); break;
      case IREM: WITH_EXCEPTION_CHECK(rem<int32_t>()); break;
      case LREM: WITH_EXCEPTION_CHECK(rem<int64_t>()); break;
      case FREM: WITH_EXCEPTION_CHECK(rem<float>()); break;
      case DREM: WITH_EXCEPTION_CHECK(rem<double>()); break;
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
        types::u1 index = mCurrentFrame->readU1();
        auto constValue = static_cast<int32_t>(std::bit_cast<int8_t>(mCurrentFrame->readU1()));

        mCurrentFrame->storeValue<int32_t>(index, mCurrentFrame->loadValue<int32_t>(index) + constValue);

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
      case F2D: mCurrentFrame->pushOperand<double>(mCurrentFrame->popOperand<float>()); break;
      case D2I: castFloatToInt<double, int32_t>(); break;
      case D2L: castFloatToInt<double, int64_t>(); break;
      case D2F: mCurrentFrame->pushOperand<float>(static_cast<float>(mCurrentFrame->popOperand<double>())); break;
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
      case IFEQ: unaryJumpIf<int32_t, 0, std::equal_to<int32_t>>(); break;
      case IFNE: unaryJumpIf<int32_t, 0, std::not_equal_to<int32_t>>(); break;
      case IFLT: unaryJumpIf<int32_t, 0, std::less<int32_t>>(); break;
      case IFGE: unaryJumpIf<int32_t, 0, std::greater_equal<int32_t>>(); break;
      case IFGT: unaryJumpIf<int32_t, 0, std::greater<int32_t>>(); break;
      case IFLE: unaryJumpIf<int32_t, 0, std::less_equal<int32_t>>(); break;
      //==--------------------------------------------------------------------==
      // Jumps
      //==--------------------------------------------------------------------==
      case IF_ICMPEQ: binaryJumpIf<int32_t, std::equal_to<int32_t>>(); break;
      case IF_ICMPNE: binaryJumpIf<int32_t, std::not_equal_to<int32_t>>(); break;
      case IF_ICMPLT: binaryJumpIf<int32_t, std::less<int32_t>>(); break;
      case IF_ICMPGE: binaryJumpIf<int32_t, std::greater_equal<int32_t>>(); break;
      case IF_ICMPGT: binaryJumpIf<int32_t, std::greater<int32_t>>(); break;
      case IF_ICMPLE: binaryJumpIf<int32_t, std::less_equal<int32_t>>(); break;
      case IF_ACMPEQ: binaryJumpIf<Instance*, std::equal_to<Instance*>>(); break;
      case IF_ACMPNE: binaryJumpIf<Instance*, std::not_equal_to<Instance*>>(); break;
      case GOTO: {
        int64_t opcodePos = mCurrentFrame->programCounter() - 1;
        auto offset = std::bit_cast<int16_t>(mCurrentFrame->readU2());

        mCurrentFrame->set(opcodePos + offset);
        break;
      }
      // The `jsr` and `ret` instructions are deprecated, we're not going to support them
      case JSR: notImplemented(opcode); break;
      case RET: notImplemented(opcode); break;
      case TABLESWITCH: tableSwitch(); break;
      case LOOKUPSWITCH: lookupSwitch(); break;
      //==--------------------------------------------------------------------==
      // Returns
      //==--------------------------------------------------------------------==
      // TODO: This can throw IllegalMonitorStateException
      case IRETURN: return Value::from<int32_t>(mCurrentFrame->popOperand<int32_t>());
      case LRETURN: return Value::from<int64_t>(mCurrentFrame->popOperand<int64_t>());
      case FRETURN: return Value::from<float>(mCurrentFrame->popOperand<float>());
      case DRETURN: return Value::from<double>(mCurrentFrame->popOperand<double>());
      case ARETURN: return Value::from<Instance*>(mCurrentFrame->popOperand<Instance*>());
      case RETURN: return std::nullopt;
      //==--------------------------------------------------------------------==
      // Field manipulation
      //==--------------------------------------------------------------------==
      case GETSTATIC: WITH_EXCEPTION_CHECK(getStatic()) break;
      case PUTSTATIC: WITH_EXCEPTION_CHECK(putStatic()) break;
      case GETFIELD: WITH_EXCEPTION_CHECK(getField()) break;
      case PUTFIELD: WITH_EXCEPTION_CHECK(putField()) break;
      //==--------------------------------------------------------------------==
      // Invocations
      //==--------------------------------------------------------------------==
      case INVOKEVIRTUAL: WITH_EXCEPTION_CHECK(invokeVirtual()) break;
      case INVOKESPECIAL: WITH_EXCEPTION_CHECK(invokeSpecial()) break;
      case INVOKESTATIC: WITH_EXCEPTION_CHECK(invokeStatic()) break;
      case INVOKEINTERFACE: WITH_EXCEPTION_CHECK(invokeInterface()) break;
      case INVOKEDYNAMIC: notImplemented(opcode); break;
      //==--------------------------------------------------------------------==
      // OOP
      //==--------------------------------------------------------------------==
      case NEW: WITH_EXCEPTION_CHECK(newObject()); break;
      case NEWARRAY: WITH_EXCEPTION_CHECK(newArray()); break;
      case ANEWARRAY: WITH_EXCEPTION_CHECK(newReferenceArray()); break;
      case ARRAYLENGTH:
        WITH_EXCEPTION_CHECK({
          ArrayInstance* arrayRef = mCurrentFrame->popOperand<Instance*>()->toArrayInstance();
          mCurrentFrame->pushOperand<int32_t>(arrayRef->length());
        })
        break;
      case ATHROW:
        WITH_EXCEPTION_CHECK({
          auto exception = mCurrentFrame->popOperand<Instance*>();
          mThread.throwException(exception);
        })
        break;
      case CHECKCAST: WITH_EXCEPTION_CHECK(this->checkCast()); break;
      case INSTANCEOF: WITH_EXCEPTION_CHECK(this->instanceOf()); break;
      case MONITORENTER:
        WITH_EXCEPTION_CHECK({
          // FIXME
          mCurrentFrame->popOperand<Instance*>();
        })
        break;
      case MONITOREXIT:
        WITH_EXCEPTION_CHECK({
          // FIXME
          mCurrentFrame->popOperand<Instance*>();
        })
        break;
      case WIDE: wide(static_cast<Opcode>(mCurrentFrame->readU1())); break;
      case MULTIANEWARRAY: WITH_EXCEPTION_CHECK(newMultiArray()); break;
      case IFNULL: unaryJumpIf<Instance*, nullptr, std::equal_to<Instance*>>(); break;
      case IFNONNULL: unaryJumpIf<Instance*, nullptr, std::not_equal_to<Instance*>>(); break;
      case GOTO_W: {
        int64_t opcodePos = mCurrentFrame->programCounter() - 1;
        auto offset = std::bit_cast<int32_t>(mCurrentFrame->readU4());

        mCurrentFrame->set(opcodePos + offset);
        break;
      }
      case JSR_W:
        // `jsr_w` is deprecated, we're not going to support it
        notImplemented(opcode);
        break;
      case NOP:
        // Nothing to do
      case BREAKPOINT:
      case IMPDEP1:
      case IMPDEP2:
        // Reserved opcodes
        break;
      default: GEEVM_UNREACHBLE("Unknown opcode");
    }
  }

  GEEVM_UNREACHBLE("Interpreter unexepectedly broke execution loop");
}

std::optional<types::u2> DefaultInterpreter::tryHandleException(GcRootRef<> exception, const Code& code, size_t pc)
{
  for (auto& entry : code.exceptionTable()) {
    if (pc < entry.startPc || pc >= entry.endPc) {
      // The exception handler is not active
      continue;
    }

    bool caught = false;
    if (entry.catchType != 0) {
      auto exceptionClass = currentFrame().currentClass()->runtimeConstantPool().getClass(entry.catchType);

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

void DefaultInterpreter::ldc(types::u2 index)
{
  auto& runtimeConstantPool = currentFrame().currentClass()->runtimeConstantPool();
  auto& [tag, data] = currentFrame().currentClass()->constantPool().getEntry(index);

  if (tag == ConstantPool::Tag::CONSTANT_Integer) {
    currentFrame().pushOperand<int32_t>(data.singleInteger);
  } else if (tag == ConstantPool::Tag::CONSTANT_Float) {
    currentFrame().pushOperand<float>(data.singleFloat);
  } else if (tag == ConstantPool::Tag::CONSTANT_String) {
    currentFrame().pushOperand<Instance*>(runtimeConstantPool.getString(index).get());
  } else if (tag == ConstantPool::Tag::CONSTANT_Class) {
    auto klass = runtimeConstantPool.getClass(index);
    // TODO: Check if class is loaded
    currentFrame().pushOperand<Instance*>((*klass)->classInstance().get());
  } else {
    GEEVM_UNREACHBLE("Unknown LDC/LDC_W type!");
  }
}

void DefaultInterpreter::ldc2_w(types::u2 index)
{
  auto& entry = mCurrentFrame->currentClass()->constantPool().getEntry(index);
  [[maybe_unused]] auto tag = entry.tag;

  assert(tag == ConstantPool::Tag::CONSTANT_Double || tag == ConstantPool::Tag::CONSTANT_Long);
  uint64_t* target = mCurrentFrame->topOfStack();

  *target = std::bit_cast<uint64_t>(entry.data);
  mCurrentFrame->advanceStackPointer(2);
}

void DefaultInterpreter::invoke(JMethod* method)
{
  auto returnValue = mThread.invoke(method);
  assert((method->isVoid() || mThread.currentException() != nullptr) || returnValue.has_value());

  if (returnValue.has_value()) {
    if (!method->isVoid()) {
      mThread.currentFrame().pushGenericOperand(returnValue->toRaw());
      if (method->descriptor().returnType().getType().isCategoryTwo()) {
        mThread.currentFrame().pushGenericOperand(0);
      }
    }
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

  uint64_t* target = mCurrentFrame->topOfStack() - 1;
  // auto arrayRef = currentFrame().popOperand<Instance*>();

  auto arrayRef = *reinterpret_cast<Instance**>(target);
  if (arrayRef == nullptr) [[unlikely]] {
    mThread.throwException(u"java/lang/NullPointerException");
    return;
  }

  JavaArray<T>* array = arrayRef->toArray<T>();
  if (index < 0 || index >= array->length()) [[unlikely]] {
    mThread.throwException(u"java/lang/ArrayIndexOutOfBoundsException");
    return;
  }

  auto element = (*array)[index];

  if constexpr (StoredAsInt<T>) {
    *target = static_cast<uint64_t>(std::bit_cast<uint32_t>(static_cast<int32_t>(element)));
    // currentFrame().pushOperand<int32_t>(static_cast<int32_t>(element));
  } else {
    using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
    *target = static_cast<uint64_t>(std::bit_cast<U>(element));
    // currentFrame().pushOperand<T>(element);
  }

  if constexpr (CategoryTwoJvmType<T>) {
    currentFrame().advanceStackPointer(1);
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

  JavaArray<T>* array = arrayRef->toArray<T>();

  if constexpr (std::is_same_v<std::remove_const_t<T>, Instance*>) {
    if (value != nullptr) {
      const JClass* elementClass = value->getClass();
      auto arrayElementClass = array->getClass()->asArrayClass()->elementClass();
      assert(arrayElementClass);

      if (!elementClass->isInstanceOf(*arrayElementClass)) {
        mThread.throwException(u"java/lang/ArrayStoreException");
        return;
      }
    }
  }

  JvmExpected<void> result = array->setArrayElement(index, value);
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

template<CategoryOneJvmType T, class F>
void DefaultInterpreter::simpleOp()
{
  T value2 = currentFrame().popOperand<T>();
  uint64_t* target = currentFrame().topOfStack() - 1;

  using U = typename unsigned_type_of_length<sizeof(T) * CHAR_BIT>::type;
  T value1 = std::bit_cast<T>(static_cast<U>(*target));

  T result = F{}(value1, value2);
  *target = static_cast<uint64_t>(std::bit_cast<U>(result));
}

template<CategoryTwoJvmType T, class F>
void DefaultInterpreter::simpleOp()
{
  T value2 = currentFrame().popOperand<T>();
  uint64_t* target = currentFrame().topOfStack() - 2;

  T result = F{}(std::bit_cast<T>(*target), value2);
  *target = std::bit_cast<uint64_t>(result);
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
void DefaultInterpreter::binaryJumpIf()
{
  auto opcodePos = currentFrame().programCounter() - 1;

  auto val2 = currentFrame().popOperand<T>();
  auto val1 = currentFrame().popOperand<T>();

  auto offset = std::bit_cast<int16_t>(currentFrame().readU2());

  if (Func{}(val1, val2)) {
    currentFrame().set(opcodePos + offset);
  }
}

template<JvmType T, auto CheckedValue, class Func>
void DefaultInterpreter::unaryJumpIf()
{
  auto opcodePos = currentFrame().programCounter() - 1;
  auto value = currentFrame().popOperand<T>();

  auto offset = std::bit_cast<int16_t>(currentFrame().readU2());

  if (Func{}(value, static_cast<T>(CheckedValue))) {
    currentFrame().set(opcodePos + offset);
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

void DefaultInterpreter::invokeVirtual()
{
  auto index = mCurrentFrame->readU2();
  const JMethod* baseMethod = currentFrame().currentClass()->runtimeConstantPool().getMethodRef(index);

  int numArgs = baseMethod->descriptor().numParameterSlots();
  auto objectRef = mCurrentFrame->peek<Instance*>(numArgs);
  if (objectRef == nullptr) {
    mThread.throwException(u"java/lang/NullPointerException");
  } else {
    JClass* target = objectRef->getClass();
    auto targetMethod = target->getVirtualMethod(baseMethod->name(), baseMethod->rawDescriptor());

    assert(targetMethod.has_value());

    this->invoke(*targetMethod);
  }
}

void DefaultInterpreter::invokeSpecial()
{
  auto index = mCurrentFrame->readU2();
  JMethod* method = currentFrame().currentClass()->runtimeConstantPool().getMethodRef(index);

  this->invoke(method);
}

void DefaultInterpreter::invokeStatic()
{
  auto index = mCurrentFrame->readU2();
  JMethod* method = currentFrame().currentClass()->runtimeConstantPool().getMethodRef(index);
  assert(method->isStatic());

  method->getClass()->initialize(mThread);
  // Initialization can fail with an exception, so only proceed if an exception did not occur
  if (mThread.currentException() == nullptr) {
    this->invoke(method);
  }
}

void DefaultInterpreter::invokeInterface()
{
  auto index = mCurrentFrame->readU2();
  const JMethod* methodRef = currentFrame().currentClass()->runtimeConstantPool().getMethodRef(index);

  // Consume 'count'
  mCurrentFrame->readU1();
  // Consume '0'
  mCurrentFrame->readU1();

  int numArgs = methodRef->descriptor().parameters().size();
  JClass* target = mCurrentFrame->peek<Instance*>(numArgs)->getClass();
  auto method = target->getVirtualMethod(methodRef->name(), methodRef->rawDescriptor());

  this->invoke(*method);
}

void DefaultInterpreter::getStatic()
{
  auto index = mCurrentFrame->readU2();
  const JField* field = currentFrame().currentClass()->runtimeConstantPool().getFieldRef(index);

  JClass* klass = field->getClass();
  klass->initialize(mThread);

  Value value = klass->getStaticFieldValue(field->offset());
  mCurrentFrame->pushGenericOperand(value.toRaw());
  if (field->fieldType().isCategoryTwo()) {
    mCurrentFrame->pushGenericOperand(0);
  }
}

void DefaultInterpreter::putStatic()
{
  auto index = mCurrentFrame->readU2();
  const JField* field = currentFrame().currentClass()->runtimeConstantPool().getFieldRef(index);

  JClass* klass = field->getClass();
  klass->initialize(mThread);

  if (field->fieldType().isCategoryTwo()) {
    mCurrentFrame->popGenericOperand();
  }
  auto value = mCurrentFrame->popGenericOperand();

  klass->setStaticFieldValue(field->offset(), value);
}

void DefaultInterpreter::getField()
{
  types::u2 index = mCurrentFrame->readU2();
  const JField* field = currentFrame().currentClass()->runtimeConstantPool().getFieldRef(index);
  auto objectRef = mCurrentFrame->popOperand<Instance*>();

  if (objectRef == nullptr) {
    mThread.throwException(u"java/lang/NullPointerException");
  } else {
    assert(objectRef->getClass()->isInstanceOf(field->getClass()));

    auto& fieldType = field->fieldType();
    fieldType.map([&]<PrimitiveType Type>() {
      using T = typename PrimitiveTypeTraits<Type>::Representation;
      auto result = objectRef->getFieldValue<T>(field->name(), field->descriptor());
      mCurrentFrame->pushOperand<T>(result);
    }, [&](types::JStringRef) {
      auto result = objectRef->getFieldValue<Instance*>(field->name(), field->descriptor());
      mCurrentFrame->pushOperand(result);
    }, [&](const ArrayType&) {
      auto result = objectRef->getFieldValue<Instance*>(field->name(), field->descriptor());
      mCurrentFrame->pushOperand(result);
    });
  }
}

void DefaultInterpreter::putField()
{
  auto index = mCurrentFrame->readU2();
  auto field = currentFrame().currentClass()->runtimeConstantPool().getFieldRef(index);

  if (field->fieldType().isCategoryTwo()) {
    mCurrentFrame->popGenericOperand();
  }
  Value value = mCurrentFrame->popGenericOperand();
  auto objectRef = mCurrentFrame->popOperand<Instance*>();

  if (objectRef == nullptr) {
    mThread.throwException(u"java/lang/NullPointerException");
    return;
  }

  assert(objectRef->getClass()->isInstanceOf(field->getClass()));

  field->fieldType().map([&]<PrimitiveType Type>() {
    using T = typename PrimitiveTypeTraits<Type>::Representation;
    if constexpr (StoredAsInt<T>) {
      objectRef->setFieldValue<T>(field->name(), field->descriptor(), static_cast<T>(value.get<int32_t>()));
    } else {
      objectRef->setFieldValue<T>(field->name(), field->descriptor(), value.get<T>());
    }
  }, [&](types::JStringRef) {
    objectRef->setFieldValue<Instance*>(field->name(), field->descriptor(), value.get<Instance*>());
  }, [&](const ArrayType&) {
    objectRef->setFieldValue<Instance*>(field->name(), field->descriptor(), value.get<Instance*>());
  });
}

void DefaultInterpreter::dup()
{
  auto value = currentFrame().popGenericOperand();
  currentFrame().pushGenericOperand(value.toRaw());
  currentFrame().pushGenericOperand(value.toRaw());
}

void DefaultInterpreter::dupX1()
{
  auto value1 = currentFrame().popGenericOperand();
  auto value2 = currentFrame().popGenericOperand();

  currentFrame().pushGenericOperand(value1.toRaw());
  currentFrame().pushGenericOperand(value2.toRaw());
  currentFrame().pushGenericOperand(value1.toRaw());
}

void DefaultInterpreter::dupX2()
{
  auto value1 = currentFrame().popGenericOperand();
  auto value2 = currentFrame().popGenericOperand();
  auto value3 = currentFrame().popGenericOperand();
  currentFrame().pushGenericOperand(value1.toRaw());
  currentFrame().pushGenericOperand(value3.toRaw());
  currentFrame().pushGenericOperand(value2.toRaw());
  currentFrame().pushGenericOperand(value1.toRaw());
}

void DefaultInterpreter::dup2()
{
  auto value1 = currentFrame().popGenericOperand();
  auto value2 = currentFrame().popGenericOperand();
  currentFrame().pushGenericOperand(value2.toRaw());
  currentFrame().pushGenericOperand(value1.toRaw());
  currentFrame().pushGenericOperand(value2.toRaw());
  currentFrame().pushGenericOperand(value1.toRaw());
}

void DefaultInterpreter::dup2X1()
{
  auto value1 = currentFrame().popGenericOperand();
  auto value2 = currentFrame().popGenericOperand();
  auto value3 = currentFrame().popGenericOperand();
  currentFrame().pushGenericOperand(value2.toRaw());
  currentFrame().pushGenericOperand(value1.toRaw());
  currentFrame().pushGenericOperand(value3.toRaw());
  currentFrame().pushGenericOperand(value2.toRaw());
  currentFrame().pushGenericOperand(value1.toRaw());
}

void DefaultInterpreter::dup2X2()
{
  auto value1 = currentFrame().popGenericOperand();
  auto value2 = currentFrame().popGenericOperand();
  auto value3 = currentFrame().popGenericOperand();
  auto value4 = currentFrame().popGenericOperand();
  currentFrame().pushGenericOperand(value2.toRaw());
  currentFrame().pushGenericOperand(value1.toRaw());
  currentFrame().pushGenericOperand(value4.toRaw());
  currentFrame().pushGenericOperand(value3.toRaw());
  currentFrame().pushGenericOperand(value2.toRaw());
  currentFrame().pushGenericOperand(value1.toRaw());
}

void DefaultInterpreter::swap()
{
  auto value1 = currentFrame().popGenericOperand();
  auto value2 = currentFrame().popGenericOperand();

  currentFrame().pushGenericOperand(value1.toRaw());
  currentFrame().pushGenericOperand(value2.toRaw());
}

void DefaultInterpreter::newObject()
{
  auto index = mCurrentFrame->readU2();
  auto className = mCurrentFrame->currentClass()->constantPool().getClassName(index);

  auto klass = mThread.resolveClass(types::JString{className});
  if (!klass) {
    this->handleErrorAsException(klass.error());
    return;
  }

  (*klass)->initialize(mThread);

  if (auto instanceClass = (*klass)->asInstanceClass(); instanceClass != nullptr) {
    Instance* instance = mThread.heap().allocate<ObjectInstance>(instanceClass);
    mCurrentFrame->pushOperand<Instance*>(instance);
  } else {
    // TODO: New with array class
    geevm_panic("new called with array class");
  }
}

void DefaultInterpreter::newArray()
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

  auto arrayType = static_cast<ArrayType>(currentFrame().readU1());
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
    default: GEEVM_UNREACHBLE("Unknown array type");
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

void DefaultInterpreter::newReferenceArray()
{
  auto index = currentFrame().readU2();
  int32_t count = currentFrame().popOperand<int32_t>();

  auto klass = currentFrame().currentClass()->runtimeConstantPool().getClass(index);
  if (!klass) {
    this->handleErrorAsException(klass.error());
    return;
  }

  types::JString elementClassName;
  if ((*klass)->isArrayType()) {
    elementClassName = u"[" + (*klass)->className();
  } else {
    elementClassName = u"[L" + (*klass)->className() + u";";
  }

  auto arrayClass = mThread.resolveClass(elementClassName);
  if (!arrayClass) {
    this->handleErrorAsException(arrayClass.error());
    return;
  }

  if (count < 0) {
    mThread.throwException(u"java/lang/NegativeArraySizeException", u"");
    return;
  }

  ArrayInstance* array = mThread.heap().allocateArray<Instance*>((*arrayClass)->asArrayClass(), count);
  currentFrame().pushOperand<Instance*>(array);
}

void DefaultInterpreter::newMultiArray()
{
  uint16_t index = currentFrame().readU2();
  uint8_t dimensions = currentFrame().readU1();

  auto klass = currentFrame().currentClass()->runtimeConstantPool().getClass(index);
  if (!klass) {
    this->handleErrorAsException(klass.error());
    return;
  }

  std::vector<int32_t> dimensionCounts;
  for (uint8_t dim = 0; dim < dimensions; dim++) {
    dimensionCounts.push_back(currentFrame().popOperand<int32_t>());
  }

  std::optional<JClass*> elementClass = (*klass)->asArrayClass()->elementClass();
  assert(elementClass.has_value());

  auto makeInnerArray = [this](auto& self, std::vector<int32_t> dimensionCounts, ArrayClass* arrayClass) -> GcRootRef<ArrayInstance> {
    auto count = dimensionCounts.back();
    dimensionCounts.pop_back();

    GcRootRef<ArrayInstance> newArray = nullptr;
    if (!dimensionCounts.empty()) {
      auto outerArray = mThread.heap().gc().pin(mThread.heap().allocateArray<Instance*>(arrayClass, count)).release();
      ArrayClass* innerArrayClass = (*arrayClass->elementClass())->asArrayClass();
      for (int32_t i = 0; i < count; i++) {
        auto innerArray = self(self, dimensionCounts, innerArrayClass);
        outerArray->setArrayElement(i, innerArray.get());
        mThread.heap().gc().release(innerArray);
      }
      newArray = outerArray;
    } else {
      newArray = mThread.heap().gc().pin(mThread.heap().allocateArray(arrayClass, count)).release();
    }

    return newArray;
  };

  GcRootRef<ArrayInstance> result = makeInnerArray(makeInnerArray, dimensionCounts, (*klass)->asArrayClass());
  currentFrame().pushOperand<Instance*>(result.get());
  mThread.heap().gc().release(result);
}

void DefaultInterpreter::checkCast()
{
  types::u2 index = mCurrentFrame->readU2();
  auto objectRef = mCurrentFrame->popOperand<Instance*>();
  if (objectRef == nullptr) {
    mCurrentFrame->pushOperand<Instance*>(objectRef);
    return;
  }

  auto klass = currentFrame().currentClass()->runtimeConstantPool().getClass(index);
  if (!klass) {
    this->handleErrorAsException(klass.error());
    return;
  }

  JClass* classToCheck = objectRef->getClass();
  if (!classToCheck->isInstanceOf(*klass)) {
    types::JString message = u"class " + classToCheck->javaClassName() + u" cannot be cast to class " + (*klass)->javaClassName();
    mThread.throwException(u"java/lang/ClassCastException", message);
  } else {
    mCurrentFrame->pushOperand<Instance*>(objectRef);
  }
}

void DefaultInterpreter::instanceOf()
{
  auto index = mCurrentFrame->readU2();
  ScopedGcRootRef<> objectRef = mThread.heap().gc().pin(mCurrentFrame->popOperand<Instance*>());
  if (objectRef == nullptr) {
    mCurrentFrame->pushOperand<int32_t>(0);
    return;
  }

  auto klass = currentFrame().currentClass()->runtimeConstantPool().getClass(index);
  if (!klass) {
    this->handleErrorAsException(klass.error());
    return;
  }

  JClass* classToCheck = objectRef->getClass();
  if (classToCheck->isInstanceOf(*klass)) {
    mCurrentFrame->pushOperand<int32_t>(1);
  } else {
    mCurrentFrame->pushOperand<int32_t>(0);
  }
}

void DefaultInterpreter::lookupSwitch()
{
  CallFrame& frame = currentFrame();

  auto opcodePos = mCurrentFrame->programCounter() - 1;
  while (mCurrentFrame->programCounter() % 4 != 0) {
    mCurrentFrame->next();
  }

  int32_t defaultOffset = std::bit_cast<int32_t>(mCurrentFrame->readU4());
  int32_t numPairs = std::bit_cast<int32_t>(mCurrentFrame->readU4());

  std::vector<std::pair<int32_t, int32_t>> pairs;
  for (int32_t i = 0; i < numPairs; i++) {
    auto matchValue = mCurrentFrame->readU4();
    auto offset = std::bit_cast<int32_t>(mCurrentFrame->readU4());
    pairs.emplace_back(matchValue, offset);
  }

  auto key = currentFrame().popOperand<int32_t>();
  bool matched = false;

  for (int32_t i = 0; i < numPairs; i++) {
    if (pairs[i].first == key) {
      mCurrentFrame->set(opcodePos + pairs[i].second);
      matched = true;
      break;
    }
  }

  if (!matched) {
    mCurrentFrame->set(opcodePos + defaultOffset);
  }
}

void DefaultInterpreter::tableSwitch()
{
  CallFrame& frame = currentFrame();
  auto opcodePos = mCurrentFrame->programCounter() - 1;
  while (mCurrentFrame->programCounter() % 4 != 0) {
    mCurrentFrame->next();
  }

  auto defaultOffset = std::bit_cast<int32_t>(mCurrentFrame->readU4());
  auto low = std::bit_cast<int32_t>(mCurrentFrame->readU4());
  auto high = std::bit_cast<int32_t>(mCurrentFrame->readU4());
  assert(low <= high);

  int32_t count = high - low + 1;

  std::vector<int32_t> table;
  table.reserve(count);

  for (int32_t i = 0; i < count; i++) {
    int32_t offset = std::bit_cast<int32_t>(mCurrentFrame->readU4());
    table.push_back(offset);
  }

  auto index = currentFrame().popOperand<int32_t>();
  if (index < low || index > high) {
    mCurrentFrame->set(opcodePos + defaultOffset);
  } else {
    int32_t targetOffset = table.at(index - low);
    mCurrentFrame->set(opcodePos + targetOffset);
  }
}

void DefaultInterpreter::wide(Opcode modifiedOpcode)
{
  types::u2 index = currentFrame().readU2();
  switch (modifiedOpcode) {
    using enum Opcode;
    case IINC: {
      int16_t constant = static_cast<int16_t>(currentFrame().readU2());
      int32_t value = currentFrame().loadValue<int32_t>(index);
      currentFrame().storeValue<int32_t>(index, value + constant);
      break;
    }
    case ILOAD: loadAndPush<int32_t>(index); break;
    case LLOAD: loadAndPush<int64_t>(index); break;
    case FLOAD: loadAndPush<float>(index); break;
    case DLOAD: loadAndPush<double>(index); break;
    case ALOAD: loadAndPush<Instance*>(index); break;
    case ISTORE: popAndStore<int32_t>(index); break;
    case LSTORE: popAndStore<int64_t>(index); break;
    case FSTORE: popAndStore<float>(index); break;
    case DSTORE: popAndStore<double>(index); break;
    case ASTORE: popAndStore<Instance*>(index); break;
    default: GEEVM_UNREACHBLE("Unknown modified opcode for WIDE");
  }
}

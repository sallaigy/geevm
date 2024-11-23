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

  void integerComparison(Predicate predicate, CodeCursor& cursor, CallFrame& frame);
  void integerComparisonToZero(Predicate predicate, CodeCursor& cursor, CallFrame& frame);

  std::optional<types::u2> tryHandleException(Instance* exception, RuntimeConstantPool& rt, const Code& code, size_t pc);

  template<JvmType T>
  void loadAndPush(CallFrame& frame, types::u2 index)
  {
    T val = frame.loadValue<T>(index);
    frame.pushOperand<T>(val);
  }

  template<JvmType T>
  void popAndStore(CallFrame& frame, types::u2 index)
  {
    T val = frame.popOperand<T>();
    frame.storeValue<T>(index, val);
  }

  template<JvmType T>
  void arrayLoad(CallFrame& frame);

  template<JvmType T>
  void arrayStore(CallFrame& frame);

  template<JvmType T>
  void add(CallFrame& frame);

  template<JvmType T>
  void sub(CallFrame& frame);

  template<JvmType T>
  void mul(CallFrame& frame);

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
      case Opcode::NOP: notImplemented(opcode); break;
      case Opcode::ACONST_NULL: frame.pushOperand<Instance*>(nullptr); break;
      case Opcode::ICONST_M1: frame.pushOperand<int32_t>(-1); break;
      case Opcode::ICONST_0: frame.pushOperand<int32_t>(0); break;
      case Opcode::ICONST_1: frame.pushOperand<int32_t>(1); break;
      case Opcode::ICONST_2: frame.pushOperand<int32_t>(2); break;
      case Opcode::ICONST_3: frame.pushOperand<int32_t>(3); break;
      case Opcode::ICONST_4: frame.pushOperand<int32_t>(4); break;
      case Opcode::ICONST_5: frame.pushOperand<int32_t>(5); break;
      case Opcode::LCONST_0: frame.pushOperand<int64_t>(0); break;
      case Opcode::LCONST_1: frame.pushOperand<int64_t>(1); break;
      case Opcode::FCONST_0: frame.pushOperand<float>(0.0f); break;
      case Opcode::FCONST_1: frame.pushOperand<float>(1.0f); break;
      case Opcode::FCONST_2: frame.pushOperand<float>(2.0f); break;
      case Opcode::DCONST_0: frame.pushOperand<double>(0.0); break;
      case Opcode::DCONST_1: frame.pushOperand<double>(1.0); break;
      case Opcode::BIPUSH: {
        auto byte = std::bit_cast<int8_t>(cursor.readU1());
        frame.pushOperand<int32_t>(static_cast<int32_t>(byte));
        break;
      }
      case Opcode::SIPUSH: {
        auto value = static_cast<int32_t>(std::bit_cast<int16_t>(cursor.readU2()));
        frame.pushOperand<int32_t>(value);
        break;
      }
      case Opcode::LDC: {
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
      case Opcode::LDC_W: {
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
      case Opcode::LDC2_W: {
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
      case Opcode::ILOAD: loadAndPush<int32_t>(frame, cursor.readU1()); break;
      case Opcode::LLOAD: loadAndPush<int64_t>(frame, cursor.readU1()); break;
      case Opcode::FLOAD: loadAndPush<float>(frame, cursor.readU1()); break;
      case Opcode::DLOAD: loadAndPush<double>(frame, cursor.readU1()); break;
      case Opcode::ALOAD: loadAndPush<Instance*>(frame, cursor.readU1()); break;
      case Opcode::ILOAD_0: loadAndPush<int32_t>(frame, 0); break;
      case Opcode::ILOAD_1: loadAndPush<int32_t>(frame, 1); break;
      case Opcode::ILOAD_2: loadAndPush<int32_t>(frame, 2); break;
      case Opcode::ILOAD_3: loadAndPush<int32_t>(frame, 3); break;
      case Opcode::LLOAD_0: loadAndPush<int64_t>(frame, 0); break;
      case Opcode::LLOAD_1: loadAndPush<int64_t>(frame, 1); break;
      case Opcode::LLOAD_2: loadAndPush<int64_t>(frame, 2); break;
      case Opcode::LLOAD_3: loadAndPush<int64_t>(frame, 3); break;
      case Opcode::FLOAD_0: loadAndPush<float>(frame, 0); break;
      case Opcode::FLOAD_1: loadAndPush<float>(frame, 1); break;
      case Opcode::FLOAD_2: loadAndPush<float>(frame, 2); break;
      case Opcode::FLOAD_3: loadAndPush<float>(frame, 3); break;
      case Opcode::DLOAD_0: loadAndPush<double>(frame, 0); break;
      case Opcode::DLOAD_1: loadAndPush<double>(frame, 1); break;
      case Opcode::DLOAD_2: loadAndPush<double>(frame, 2); break;
      case Opcode::DLOAD_3: loadAndPush<double>(frame, 3); break;
      case Opcode::ALOAD_0: loadAndPush<Instance*>(frame, 0); break;
      case Opcode::ALOAD_1: loadAndPush<Instance*>(frame, 1); break;
      case Opcode::ALOAD_2: loadAndPush<Instance*>(frame, 2); break;
      case Opcode::ALOAD_3: loadAndPush<Instance*>(frame, 3); break;
      case Opcode::IALOAD: arrayLoad<int32_t>(frame); break;
      case Opcode::LALOAD: arrayLoad<int64_t>(frame); break;
      case Opcode::FALOAD: arrayLoad<float>(frame); break;
      case Opcode::DALOAD: arrayLoad<double>(frame); break;
      case Opcode::AALOAD: arrayLoad<Instance*>(frame); break;
      case Opcode::BALOAD: arrayLoad<int8_t>(frame); break;
      case Opcode::CALOAD: arrayLoad<char16_t>(frame); break;
      case Opcode::SALOAD: arrayLoad<int16_t>(frame); break;
      case Opcode::ISTORE: {
        auto index = cursor.readU1();
        frame.storeValue(index, frame.popOperand<int32_t>());
        break;
      }
      case Opcode::LSTORE: {
        auto index = cursor.readU1();
        frame.storeValue<int64_t>(index, frame.popOperand<int64_t>());
        break;
      }
      case Opcode::FSTORE: notImplemented(opcode); break;
      case Opcode::DSTORE: notImplemented(opcode); break;
      case Opcode::ASTORE: {
        auto index = cursor.readU1();
        frame.storeValue(index, frame.popOperand<Instance*>());
        break;
      }
      case Opcode::ISTORE_0: popAndStore<int32_t>(frame, 0); break;
      case Opcode::ISTORE_1: popAndStore<int32_t>(frame, 1); break;
      case Opcode::ISTORE_2: popAndStore<int32_t>(frame, 2); break;
      case Opcode::ISTORE_3: popAndStore<int32_t>(frame, 3); break;
      case Opcode::LSTORE_0: popAndStore<int64_t>(frame, 0); break;
      case Opcode::LSTORE_1: popAndStore<int64_t>(frame, 1); break;
      case Opcode::LSTORE_2: popAndStore<int64_t>(frame, 2); break;
      case Opcode::LSTORE_3: popAndStore<int64_t>(frame, 3); break;
      case Opcode::FSTORE_0: popAndStore<float>(frame, 0); break;
      case Opcode::FSTORE_1: popAndStore<float>(frame, 1); break;
      case Opcode::FSTORE_2: popAndStore<float>(frame, 2); break;
      case Opcode::FSTORE_3: popAndStore<float>(frame, 3); break;
      case Opcode::DSTORE_0: popAndStore<double>(frame, 0); break;
      case Opcode::DSTORE_1: popAndStore<double>(frame, 1); break;
      case Opcode::DSTORE_2: popAndStore<double>(frame, 2); break;
      case Opcode::DSTORE_3: popAndStore<double>(frame, 3); break;
      case Opcode::ASTORE_0: popAndStore<Instance*>(frame, 0); break;
      case Opcode::ASTORE_1: popAndStore<Instance*>(frame, 1); break;
      case Opcode::ASTORE_2: popAndStore<Instance*>(frame, 2); break;
      case Opcode::ASTORE_3: popAndStore<Instance*>(frame, 3); break;
      // Array store
      //==--------------------------------------------------------------------==
      case Opcode::IASTORE: arrayStore<int32_t>(frame); break;
      case Opcode::LASTORE: arrayStore<int64_t>(frame); break;
      case Opcode::FASTORE: arrayStore<float>(frame); break;
      case Opcode::DASTORE: arrayStore<double>(frame); break;
      case Opcode::AASTORE: {
        Instance* value = frame.popOperand<Instance*>();
        int32_t index = frame.popOperand<int32_t>();
        Instance* arrayRef = frame.popOperand<Instance*>();

        if (arrayRef == nullptr) {
          mThread.throwException(u"java/lang/NullPointerException");
          break;
        }

        ArrayInstance* array = arrayRef->asArrayInstance();

        if (value != nullptr) {
          JClass* elementClass = value->getClass();
          auto arrayElementClass = array->getClass()->asArrayClass()->elementClass();
          assert(arrayElementClass);

          if (!elementClass->isInstanceOf(*arrayElementClass)) {
            mThread.throwException(u"java/lang/ArrayStoreException");
            break;
          }
        }

        if (auto res = array->setArrayElement(index, Value::from<Instance*>(value)); !res) {
          this->handleErrorAsException(res.error());
          break;
        }

        break;
      }
      case Opcode::BASTORE: {
        int32_t value = frame.popOperand<int32_t>();
        int32_t index = frame.popOperand<int32_t>();
        Instance* arrayRef = frame.popOperand<Instance*>();

        if (arrayRef == nullptr) {
          mThread.throwException(u"java/lang/NullPointerException");
          break;
        }

        ArrayInstance* array = arrayRef->asArrayInstance();

        int8_t byteValue = static_cast<int8_t>(value & 0x000000FF);
        if (auto res = array->setArrayElement<int8_t>(index, byteValue); !res) {
          this->handleErrorAsException(res.error());
          break;
        }

        break;
      }
      case Opcode::CASTORE: {
        int32_t value = frame.popOperand<int32_t>();
        int32_t index = frame.popOperand<int32_t>();
        Instance* arrayRef = frame.popOperand<Instance*>();

        if (arrayRef == nullptr) {
          mThread.throwException(u"java/lang/NullPointerException");
          break;
        }

        ArrayInstance* array = arrayRef->asArrayInstance();

        if (auto res = array->setArrayElement<char16_t>(index, static_cast<char16_t>(value)); !res) {
          this->handleErrorAsException(res.error());
          break;
        }

        break;
      }
      case Opcode::SASTORE: {
        int32_t value = frame.popOperand<int32_t>();
        int32_t index = frame.popOperand<int32_t>();
        Instance* arrayRef = frame.popOperand<Instance*>();

        if (arrayRef == nullptr) {
          mThread.throwException(u"java/lang/NullPointerException");
          break;
        }

        ArrayInstance* array = arrayRef->asArrayInstance();

        int16_t shortValue = static_cast<int16_t>(value & 0x0000FFFF);
        if (auto res = array->setArrayElement<int16_t>(index, shortValue); !res) {
          this->handleErrorAsException(res.error());
          break;
        }

        break;
      }
      case Opcode::POP: {
        frame.popGenericOperand();
        break;
      }
      case Opcode::POP2: notImplemented(opcode); break;
      case Opcode::DUP: {
        // TOOD: Duplicate instead of pop / push
        auto value = frame.popGenericOperand();
        frame.pushGenericOperand(value);
        frame.pushGenericOperand(value);
        break;
      }
      case Opcode::DUP_X1: {
        Value value1 = frame.popGenericOperand();
        Value value2 = frame.popGenericOperand();

        frame.pushGenericOperand(value1);
        frame.pushGenericOperand(value2);
        frame.pushGenericOperand(value1);

        break;
      }
      case Opcode::DUP_X2: notImplemented(opcode); break;
      case Opcode::DUP2: {
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
      case Opcode::DUP2_X1: notImplemented(opcode); break;
      case Opcode::DUP2_X2: notImplemented(opcode); break;
      case Opcode::SWAP: notImplemented(opcode); break;
      case Opcode::IADD: add<int32_t>(frame); break;
      case Opcode::LADD: add<int64_t>(frame); break;
      case Opcode::FADD: add<float>(frame); break;
      case Opcode::DADD: add<double>(frame); break;
      case Opcode::ISUB: sub<int32_t>(frame); break;
      case Opcode::LSUB: sub<int64_t>(frame); break;
      case Opcode::FSUB: sub<float>(frame); break;
      case Opcode::DSUB: sub<double>(frame); break;
      case Opcode::IMUL: mul<int32_t>(frame); break;
      case Opcode::LMUL: mul<int64_t>(frame); break;
      case Opcode::FMUL: mul<float>(frame); break;
      case Opcode::DMUL: mul<double>(frame); break;
      case Opcode::IDIV: {
        int32_t value2 = frame.popOperand<int32_t>();
        int32_t value1 = frame.popOperand<int32_t>();

        if (value2 == 0) {
          mThread.throwException(u"java/lang/ArithmeticException", u"Divison by zero");
          break;
        }

        frame.pushOperand<int32_t>(value1 / value2);
        break;
      }
      case Opcode::LDIV: {
        int64_t value2 = frame.popOperand<int64_t>();
        int64_t value1 = frame.popOperand<int64_t>();

        if (value2 == 0) {
          mThread.throwException(u"java/lang/ArithmeticException", u"Divison by zero");
          break;
        }

        frame.pushOperand<int64_t>(value1 / value2);

        break;
      }
      case Opcode::FDIV: {
        float value2 = frame.popOperand<float>();
        float value1 = frame.popOperand<float>();

        // TODO: Value set conversion

        float result = value1 / value2;

        frame.pushOperand<float>(result);

        break;
      }
      case Opcode::DDIV: {
        double value2 = frame.popOperand<double>();
        double value1 = frame.popOperand<double>();

        // TODO: Value set conversion

        double result = value1 / value2;

        frame.pushOperand<double>(result);

        break;
      }
      case Opcode::IREM: {
        int32_t value2 = frame.popOperand<int32_t>();
        int32_t value1 = frame.popOperand<int32_t>();

        int32_t result = value1 - (value1 / value2) * value2;

        frame.pushOperand<int32_t>(result);
        break;
      }
      case Opcode::LREM: {
        int64_t value2 = frame.popOperand<int64_t>();
        int64_t value1 = frame.popOperand<int64_t>();

        int64_t result = value1 - (value1 / value2) * value2;

        frame.pushOperand<int64_t>(result);
        break;
      }
      case Opcode::FREM: {
        float value2 = frame.popOperand<float>();
        float value1 = frame.popOperand<float>();

        // TODO: Value set conversion

        float result = std::fmod(value1, value2);

        frame.pushOperand<float>(result);

        break;
      }
      case Opcode::DREM: {
        double value2 = frame.popOperand<double>();
        double value1 = frame.popOperand<double>();

        // TODO: Value set conversion

        double result = std::fmod(value1, value2);

        frame.pushOperand<double>(result);

        break;
      }
      case Opcode::INEG: notImplemented(opcode); break;
      case Opcode::LNEG: notImplemented(opcode); break;
      case Opcode::FNEG: notImplemented(opcode); break;
      case Opcode::DNEG: notImplemented(opcode); break;
      case Opcode::ISHL: {
        int32_t value2 = frame.popOperand<int32_t>();
        int32_t value1 = frame.popOperand<int32_t>();

        uint32_t offset = std::bit_cast<uint32_t>(value2) & 0x0000001F;

        frame.pushOperand<int32_t>((value1 << offset));

        break;
      }
      case Opcode::LSHL: {
        int32_t value2 = frame.popOperand<int32_t>();
        int64_t value1 = frame.popOperand<int64_t>();

        uint32_t offset = std::bit_cast<uint32_t>(value2) & 0x0000003F;

        frame.pushOperand<int64_t>((value1 << offset));

        break;
      }
      case Opcode::ISHR: notImplemented(opcode); break;
      case Opcode::LSHR: notImplemented(opcode); break;
      case Opcode::IUSHR: {
        int32_t value2 = frame.popOperand<int32_t>();
        int32_t value1 = frame.popOperand<int32_t>();

        uint32_t offset = std::bit_cast<uint32_t>(value2) & 0x0000001F;

        // TODO: Is this working according to spec?
        frame.pushOperand<int32_t>((value1 >> offset));
        break;
      }
      case Opcode::LUSHR: notImplemented(opcode); break;
      case Opcode::IAND: {
        int32_t value2 = frame.popOperand<int32_t>();
        int32_t value1 = frame.popOperand<int32_t>();

        frame.pushOperand<int32_t>((value1 & value2));
        break;
      }
      case Opcode::LAND: {
        int64_t value2 = frame.popOperand<int64_t>();
        int64_t value1 = frame.popOperand<int64_t>();

        frame.pushOperand<int64_t>((value1 & value2));
        break;
      }
      case Opcode::IOR: notImplemented(opcode); break;
      case Opcode::LOR: notImplemented(opcode); break;
      case Opcode::IXOR: {
        int32_t value2 = frame.popOperand<int32_t>();
        int32_t value1 = frame.popOperand<int32_t>();

        frame.pushOperand<int32_t>((value1 ^ value2));

        break;
      }
      case Opcode::LXOR: notImplemented(opcode); break;
      case Opcode::IINC: {
        types::u1 index = cursor.readU1();
        auto constValue = static_cast<int32_t>(std::bit_cast<int8_t>(cursor.readU1()));

        frame.storeValue<int32_t>(index, frame.loadValue<int32_t>(index) + constValue);

        break;
      }
      case Opcode::I2L: {
        int32_t value = frame.popOperand<int32_t>();
        frame.pushOperand<int64_t>((static_cast<int64_t>(value)));
        break;
      }
      case Opcode::I2F: {
        int32_t value = frame.popOperand<int32_t>();
        frame.pushOperand<float>((static_cast<float>(value)));
        break;
      }
      case Opcode::I2D: {
        int32_t value = frame.popOperand<int32_t>();
        frame.pushOperand<double>((static_cast<double>(value)));
        break;
      }
      case Opcode::L2I: notImplemented(opcode); break;
      case Opcode::L2F: notImplemented(opcode); break;
      case Opcode::L2D: notImplemented(opcode); break;
      case Opcode::F2I: {
        // TODO: Is this ok according to spec?
        float value = frame.popOperand<float>();
        frame.pushOperand<int32_t>((static_cast<int32_t>(value)));
        break;
      }
      case Opcode::F2L: notImplemented(opcode); break;
      case Opcode::F2D: notImplemented(opcode); break;
      case Opcode::D2I: notImplemented(opcode); break;
      case Opcode::D2L: notImplemented(opcode); break;
      case Opcode::D2F: notImplemented(opcode); break;
      case Opcode::I2B: {
        int32_t value = frame.popOperand<int32_t>();
        int8_t byteValue = static_cast<int8_t>(value & 0x000000FF);

        frame.pushOperand<int32_t>((static_cast<int32_t>(byteValue)));

        break;
      }
      case Opcode::I2C: {
        int32_t value = frame.popOperand<int32_t>();
        char16_t charValue = static_cast<char16_t>(value);

        frame.pushOperand<int32_t>(charValue);
        break;
      }
      case Opcode::I2S: {
        int32_t value = frame.popOperand<int32_t>();
        int16_t shortValue = static_cast<int16_t>(value & 0x0000FFFF);

        frame.pushOperand<int32_t>((static_cast<int32_t>(shortValue)));
        break;
      }
      case Opcode::LCMP: {
        int64_t value2 = frame.popOperand<int64_t>();
        int64_t value1 = frame.popOperand<int64_t>();

        if (value1 > value2) {
          frame.pushOperand<int32_t>(1);
        } else if (value1 < value2) {
          frame.pushOperand<int32_t>(-1);
        } else {
          frame.pushOperand<int32_t>(0);
        }

        break;
      }
      case Opcode::FCMPL: {
        float value2 = frame.popOperand<float>();
        float value1 = frame.popOperand<float>();

        if (value1 > value2) {
          frame.pushOperand<int32_t>(1);
        } else if (value1 == value2) {
          // TODO: Is this IEE equality?
          frame.pushOperand<int32_t>(0);
        } else if (value1 < value2) {
          frame.pushOperand<int32_t>((-1));
        } else {
          frame.pushOperand<int32_t>((-1));
        }
        break;
      }
      case Opcode::FCMPG: {
        float value2 = frame.popOperand<float>();
        float value1 = frame.popOperand<float>();

        if (value1 > value2) {
          frame.pushOperand<int32_t>(1);
        } else if (value1 == value2) {
          // TODO: Is this IEE equality?
          frame.pushOperand<int32_t>(0);
        } else if (value1 < value2) {
          frame.pushOperand<int32_t>((-1));
        } else {
          frame.pushOperand<int32_t>(1);
        }

        break;
      }
      case Opcode::DCMPL: notImplemented(opcode); break;
      case Opcode::DCMPG: notImplemented(opcode); break;
      case Opcode::IFEQ: integerComparisonToZero(Predicate::Eq, cursor, frame); break;
      case Opcode::IFNE: integerComparisonToZero(Predicate::NotEq, cursor, frame); break;
      case Opcode::IFLT: integerComparisonToZero(Predicate::Lt, cursor, frame); break;
      case Opcode::IFGE: integerComparisonToZero(Predicate::GtEq, cursor, frame); break;
      case Opcode::IFGT: integerComparisonToZero(Predicate::Gt, cursor, frame); break;
      case Opcode::IFLE: integerComparisonToZero(Predicate::LtEq, cursor, frame); break;
      case Opcode::IF_ICMPEQ: integerComparison(Predicate::Eq, cursor, frame); break;
      case Opcode::IF_ICMPNE: integerComparison(Predicate::NotEq, cursor, frame); break;
      case Opcode::IF_ICMPLT: integerComparison(Predicate::Lt, cursor, frame); break;
      case Opcode::IF_ICMPGE: integerComparison(Predicate::GtEq, cursor, frame); break;
      case Opcode::IF_ICMPGT: integerComparison(Predicate::Gt, cursor, frame); break;
      case Opcode::IF_ICMPLE: integerComparison(Predicate::LtEq, cursor, frame); break;
      case Opcode::IF_ACMPEQ: {
        auto opcodePos = cursor.position() - 1;

        Instance* value2 = frame.popOperand<Instance*>();
        Instance* value1 = frame.popOperand<Instance*>();

        auto offset = std::bit_cast<int16_t>(cursor.readU2());

        if (value1 == value2) {
          cursor.set(opcodePos + offset);
        }

        break;
      }
      case Opcode::IF_ACMPNE: {
        auto opcodePos = cursor.position() - 1;

        Instance* value2 = frame.popOperand<Instance*>();
        Instance* value1 = frame.popOperand<Instance*>();

        auto offset = std::bit_cast<int16_t>(cursor.readU2());

        if (value1 != value2) {
          cursor.set(opcodePos + offset);
        }

        break;
      }
      case Opcode::GOTO: {
        auto opcodePos = cursor.position() - 1;
        auto offset = std::bit_cast<int16_t>(cursor.readU2());

        cursor.set(opcodePos + offset);
        break;
      }
      case Opcode::JSR: notImplemented(opcode); break;
      case Opcode::RET: notImplemented(opcode); break;
      case Opcode::TABLESWITCH: notImplemented(opcode); break;
      case Opcode::LOOKUPSWITCH: notImplemented(opcode); break;
      case Opcode::IRETURN: return Value::from<int32_t>(frame.popOperand<int32_t>());
      case Opcode::LRETURN: return Value::from<int64_t>(frame.popOperand<int64_t>());
      case Opcode::FRETURN: return Value::from<float>(frame.popOperand<float>());
      case Opcode::DRETURN: return Value::from<double>(frame.popOperand<double>());
      case Opcode::ARETURN: return Value::from<Instance*>(frame.popOperand<Instance*>());
      case Opcode::RETURN: return std::nullopt;
      case Opcode::GETSTATIC: {
        auto index = cursor.readU2();
        JField* field = runtimeConstantPool.getFieldRef(index);

        JClass* klass = field->getClass();
        klass->initialize(mThread);

        Value value = klass->getStaticFieldValue(field->offset());
        frame.pushGenericOperand(value);

        break;
      }
      case Opcode::PUTSTATIC: {
        auto index = cursor.readU2();
        JField* field = runtimeConstantPool.getFieldRef(index);

        JClass* klass = field->getClass();
        klass->initialize(mThread);

        klass->setStaticFieldValue(field->offset(), frame.popGenericOperand());

        break;
      }
      case Opcode::GETFIELD: {
        types::u2 index = cursor.readU2();
        JField* field = runtimeConstantPool.getFieldRef(index);
        Instance* objectRef = frame.popOperand<Instance*>();

        assert(objectRef->getClass()->isInstanceOf(field->getClass()));

        // TODO: Null check

        assert(objectRef->getClass()->isInstanceOf(field->getClass()));
        frame.pushGenericOperand(objectRef->getFieldValue(field->name(), field->descriptor()));
        break;
      }
      case Opcode::PUTFIELD: {
        auto index = cursor.readU2();
        auto field = runtimeConstantPool.getFieldRef(index);

        Value value = frame.popGenericOperand();
        Instance* objectRef = frame.popOperand<Instance*>();
        assert(objectRef->getClass()->isInstanceOf(field->getClass()));

        objectRef->setFieldValue(field->name(), field->descriptor(), value);

        break;
      }
      case Opcode::INVOKEVIRTUAL: {
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
      case Opcode::INVOKESPECIAL: {
        auto index = cursor.readU2();
        JMethod* method = runtimeConstantPool.getMethodRef(index);

        this->invoke(method);

        break;
      }
      case Opcode::INVOKESTATIC: {
        auto index = cursor.readU2();
        JMethod* method = runtimeConstantPool.getMethodRef(index);
        assert(method->isStatic());

        method->getClass()->initialize(mThread);

        this->invoke(method);

        break;
      }
      case Opcode::INVOKEINTERFACE: {
        auto index = cursor.readU2();
        JMethod* methodRef = runtimeConstantPool.getMethodRef(index);

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
      case Opcode::INVOKEDYNAMIC: notImplemented(opcode); break;
      case Opcode::NEW: {
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
      case Opcode::NEWARRAY: {
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

        auto atype = static_cast<ArrayType>(cursor.readU1());
        int32_t count = frame.popOperand<int32_t>();

        types::JString arrayClsName;

        switch (atype) {
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
          break;
        }

        if (count < 0) {
          mThread.throwException(u"java/lang/NegativeArraySizeException", u"");
          break;
        }

        ArrayInstance* newInstance = mThread.heap().allocateArray((*arrayClass)->asArrayClass(), count);
        frame.pushOperand<Instance*>(newInstance);

        break;
      }
      case Opcode::ANEWARRAY: {
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
      case Opcode::ARRAYLENGTH: {
        ArrayInstance* arrayRef = frame.popOperand<Instance*>()->asArrayInstance();
        frame.pushOperand<int32_t>(arrayRef->length());
        break;
      }
      case Opcode::ATHROW: {
        auto exception = frame.popOperand<Instance*>();
        mThread.throwException(exception);
        break;
      }
      case Opcode::CHECKCAST: {
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
      case Opcode::INSTANCEOF: {
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
      case Opcode::MONITORENTER: {
        // FIXME
        frame.popOperand<Instance*>();
        break;
      }
      case Opcode::MONITOREXIT: {
        // FIXME
        frame.popOperand<Instance*>();
        break;
      }

      case Opcode::WIDE: notImplemented(opcode); break;
      case Opcode::MULTIANEWARRAY: notImplemented(opcode); break;
      case Opcode::IFNULL: {
        auto opcodePos = cursor.position() - 1;
        auto offset = std::bit_cast<int16_t>(cursor.readU2());

        Instance* value = frame.popOperand<Instance*>();
        if (value == nullptr) {
          cursor.set(opcodePos + offset);
        }

        break;
      }
      case Opcode::IFNONNULL: {
        auto opcodePos = cursor.position() - 1;
        auto offset = std::bit_cast<int16_t>(cursor.readU2());

        Instance* value = frame.popOperand<Instance*>();
        if (value != nullptr) {
          cursor.set(opcodePos + offset);
        }

        break;
      }
      case Opcode::GOTO_W: notImplemented(opcode); break;
      case Opcode::JSR_W: notImplemented(opcode); break;
      case Opcode::BREAKPOINT: notImplemented(opcode); break;
      case Opcode::IMPDEP1: notImplemented(opcode); break;
      case Opcode::IMPDEP2: notImplemented(opcode); break;
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

void DefaultInterpreter::integerComparison(Predicate predicate, CodeCursor& cursor, CallFrame& frame)
{
  auto opcodePos = cursor.position() - 1;

  auto val2 = frame.popOperand<int32_t>();
  auto val1 = frame.popOperand<int32_t>();

  auto offset = std::bit_cast<int16_t>(cursor.readU2());

  if (compareInt(predicate, val1, val2)) {
    cursor.set(opcodePos + offset);
  }
}

void DefaultInterpreter::integerComparisonToZero(Predicate predicate, CodeCursor& cursor, CallFrame& frame)
{
  auto opcodePos = cursor.position() - 1;

  auto val1 = frame.popOperand<int32_t>();

  auto offset = std::bit_cast<int16_t>(cursor.readU2());

  if (compareInt(predicate, val1, 0)) {
    cursor.set(opcodePos + offset);
  }
}

template<JvmType T>
void DefaultInterpreter::arrayLoad(CallFrame& frame)
{
  int32_t index = frame.popOperand<int32_t>();
  Instance* arrayRef = frame.popOperand<Instance*>();

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
    frame.pushOperand<int32_t>(static_cast<int32_t>(*element));
  } else {
    frame.pushOperand<T>(*element);
  }
}

template<JvmType T>
void DefaultInterpreter::arrayStore(CallFrame& frame)
{
  T value = frame.popOperand<T>();
  int32_t index = frame.popOperand<int32_t>();
  Instance* arrayRef = frame.popOperand<Instance*>();

  if (arrayRef == nullptr) {
    mThread.throwException(u"java/lang/NullPointerException");
    return;
  }

  ArrayInstance* array = arrayRef->asArrayInstance();
  auto result = array->setArrayElement<T>(index, value);

  if (!result) {
    mThread.throwException(u"java/lang/ArrayIndexOutOfBoundsException");
  }
}

template<JvmType T>
void DefaultInterpreter::add(CallFrame& frame)
{
  T value2 = frame.popOperand<T>();
  T value1 = frame.popOperand<T>();

  T result = value2 + value1;

  frame.pushOperand<T>(result);
}

template<JvmType T>
void DefaultInterpreter::sub(CallFrame& frame)
{
  T value2 = frame.popOperand<T>();
  T value1 = frame.popOperand<T>();

  T result = value1 - value2;

  frame.pushOperand<T>(result);
}

template<JvmType T>
void DefaultInterpreter::mul(CallFrame& frame)
{
  T value2 = frame.popOperand<T>();
  T value1 = frame.popOperand<T>();

  T result = value2 * value1;

  frame.pushOperand<T>(result);
}

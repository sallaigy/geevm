#include "vm/Interpreter.h"

#include <iostream>

#include "class_file/Code.h"
#include "class_file/Opcode.h"
#include "vm/Frame.h"
#include "vm/Instance.h"
#include "vm/Vm.h"

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
  std::optional<Value> execute(JavaThread& thread, const Code& code, std::size_t startPc) override;

private:
  void invoke(JavaThread& thread, JMethod* method);

  void integerComparison(Predicate predicate, CodeCursor& cursor, CallFrame& frame);
  void integerComparisonToZero(Predicate predicate, CodeCursor& cursor, CallFrame& frame);
};

} // namespace

std::unique_ptr<Interpreter> geevm::createDefaultInterpreter()
{
  return std::make_unique<DefaultInterpreter>();
}

static void notImplemented(Opcode opcode)
{
  throw std::runtime_error("Opcode not implemented: " + opcodeToString(opcode));
}

std::optional<Value> DefaultInterpreter::execute(JavaThread& thread, const Code& code, std::size_t startPc)
{
  CodeCursor cursor(code.bytes(), startPc);

  while (cursor.hasNext()) {
    Opcode opcode = cursor.next();
    CallFrame& frame = thread.currentFrame();
    RuntimeConstantPool& runtimeConstantPool = frame.currentClass()->runtimeConstantPool();

    switch (opcode) {
      case Opcode::NOP: notImplemented(opcode); break;
      case Opcode::ACONST_NULL: frame.pushOperand(Value::Reference(nullptr)); break;
      case Opcode::ICONST_M1: frame.pushOperand(Value::Int(-1)); break;
      case Opcode::ICONST_0: frame.pushOperand(Value::Int(0)); break;
      case Opcode::ICONST_1: frame.pushOperand(Value::Int(1)); break;
      case Opcode::ICONST_2: frame.pushOperand(Value::Int(2)); break;
      case Opcode::ICONST_3: frame.pushOperand(Value::Int(3)); break;
      case Opcode::ICONST_4: frame.pushOperand(Value::Int(4)); break;
      case Opcode::ICONST_5: frame.pushOperand(Value::Int(5)); break;
      case Opcode::LCONST_0: frame.pushOperand(Value::Long(0)); break;
      case Opcode::LCONST_1: frame.pushOperand(Value::Long(1)); break;
      case Opcode::FCONST_0: frame.pushOperand(Value::Float(0.0f)); break;
      case Opcode::FCONST_1: frame.pushOperand(Value::Float(1.0f)); break;
      case Opcode::FCONST_2: frame.pushOperand(Value::Float(2.0f)); break;
      case Opcode::DCONST_0: notImplemented(opcode); break;
      case Opcode::DCONST_1: notImplemented(opcode); break;
      case Opcode::BIPUSH: {
        auto byte = std::bit_cast<int8_t>(cursor.readU1());
        frame.pushOperand(Value::Int(static_cast<int32_t>(byte)));
        break;
      }
      case Opcode::SIPUSH: {
        auto value = static_cast<int32_t>(std::bit_cast<int16_t>(cursor.readU2()));
        frame.pushOperand(Value::Int(value));
        break;
      }
      case Opcode::LDC: {
        types::u1 index = cursor.readU1();
        auto entry = frame.currentClass()->constantPool().getEntry(index);

        if (entry.tag == ConstantPool::Tag::CONSTANT_Integer) {
          frame.pushOperand(Value::Int(entry.data.singleInteger));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Float) {
          frame.pushOperand(Value::Float(entry.data.singleFloat));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_String) {
          frame.pushOperand(Value::Reference(runtimeConstantPool.getString(index)));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Class) {
          auto klass = runtimeConstantPool.getClass(index);
          // TODO: Check if class is loaded
          frame.pushOperand(Value::Reference((*klass)->classInstance()));
        } else {
          assert(false && "Unknown LDC type!");
        }
        break;
      }
      case Opcode::LDC_W: {
        types::u2 index = cursor.readU2();
        auto& entry = frame.currentClass()->constantPool().getEntry(index);

        if (entry.tag == ConstantPool::Tag::CONSTANT_Integer) {
          frame.pushOperand(Value::Int(entry.data.singleInteger));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Float) {
          frame.pushOperand(Value::Float(entry.data.singleFloat));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_String) {
          frame.pushOperand(Value::Reference(runtimeConstantPool.getString(index)));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Class) {
          auto klass = runtimeConstantPool.getClass(index);
          // TODO: Check if class is loaded
          frame.pushOperand(Value::Reference((*klass)->classInstance()));
        } else {
          assert(false && "Unknown LDC_W type!");
        }
        break;
      }
      case Opcode::LDC2_W: {
        types::u2 index = cursor.readU2();
        auto& entry = frame.currentClass()->constantPool().getEntry(index);

        if (entry.tag == ConstantPool::Tag::CONSTANT_Double) {
          frame.pushOperand(Value::Double(entry.data.doubleFloat));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Long) {
          frame.pushOperand(Value::Long(entry.data.doubleInteger));
        } else {
          assert(false && "ldc2_w target entry must be double or long");
        }

        break;
      }
      case Opcode::ILOAD: frame.pushOperand(frame.loadValue(cursor.readU1())); break;
      case Opcode::LLOAD: {
        auto index = cursor.readU1();
        frame.pushOperand(frame.loadValue(index));
        break;
      }
      case Opcode::FLOAD: notImplemented(opcode); break;
      case Opcode::DLOAD: notImplemented(opcode); break;
      case Opcode::ALOAD: frame.pushOperand(frame.loadValue(cursor.readU1())); break;
      case Opcode::ILOAD_0: frame.pushOperand(frame.loadValue(0)); break;
      case Opcode::ILOAD_1: frame.pushOperand(frame.loadValue(1)); break;
      case Opcode::ILOAD_2: frame.pushOperand(frame.loadValue(2)); break;
      case Opcode::ILOAD_3: frame.pushOperand(frame.loadValue(3)); break;
      case Opcode::LLOAD_0: frame.pushOperand(frame.loadValue(0)); break;
      case Opcode::LLOAD_1: frame.pushOperand(frame.loadValue(1)); break;
      case Opcode::LLOAD_2: frame.pushOperand(frame.loadValue(2)); break;
      case Opcode::LLOAD_3: frame.pushOperand(frame.loadValue(3)); break;
      case Opcode::FLOAD_0: frame.pushOperand(frame.loadValue(0)); break;
      case Opcode::FLOAD_1: frame.pushOperand(frame.loadValue(1)); break;
      case Opcode::FLOAD_2: frame.pushOperand(frame.loadValue(2)); break;
      case Opcode::FLOAD_3: frame.pushOperand(frame.loadValue(3)); break;
      case Opcode::DLOAD_0: notImplemented(opcode); break;
      case Opcode::DLOAD_1: notImplemented(opcode); break;
      case Opcode::DLOAD_2: notImplemented(opcode); break;
      case Opcode::DLOAD_3: notImplemented(opcode); break;
      case Opcode::ALOAD_0: frame.pushOperand(frame.loadValue(0)); break;
      case Opcode::ALOAD_1: frame.pushOperand(frame.loadValue(1)); break;
      case Opcode::ALOAD_2: frame.pushOperand(frame.loadValue(2)); break;
      case Opcode::ALOAD_3: frame.pushOperand(frame.loadValue(3)); break;
      case Opcode::IALOAD: notImplemented(opcode); break;
      case Opcode::LALOAD: notImplemented(opcode); break;
      case Opcode::FALOAD: notImplemented(opcode); break;
      case Opcode::DALOAD: notImplemented(opcode); break;
      case Opcode::AALOAD: {
        int32_t index = frame.popOperand().asInt();
        ArrayInstance* arrayRef = frame.popOperand().asReference()->asArrayInstance();

        frame.pushOperand(*arrayRef->getArrayElement(index));
        // TODO: Check and throw exceptions

        break;
      }
      case Opcode::BALOAD: notImplemented(opcode); break;
      case Opcode::CALOAD: {
        int32_t index = frame.popOperand().asInt();
        ArrayInstance* arrayRef = frame.popOperand().asReference()->asArrayInstance();

        char16_t value = arrayRef->getArrayElement(index)->asChar();
        frame.pushOperand(Value::Int(static_cast<int32_t>(value)));
        // TODO: Check and throw exceptions

        break;
      }
      case Opcode::SALOAD: notImplemented(opcode); break;
      case Opcode::ISTORE: {
        auto index = cursor.readU1();
        frame.storeValue(index, frame.popInt());
        break;
      }
      case Opcode::LSTORE: {
        auto index = cursor.readU1();
        frame.storeLongValue(index, frame.popOperand());
        break;
      }
      case Opcode::FSTORE: notImplemented(opcode); break;
      case Opcode::DSTORE: notImplemented(opcode); break;
      case Opcode::ASTORE: {
        auto index = cursor.readU1();
        frame.storeValue(index, frame.popOperand());
        break;
      }
      case Opcode::ISTORE_0: notImplemented(opcode); break;
      case Opcode::ISTORE_1: frame.storeValue(1, frame.popInt()); break;
      case Opcode::ISTORE_2: frame.storeValue(2, frame.popInt()); break;
      case Opcode::ISTORE_3: frame.storeValue(3, frame.popInt()); break;
      case Opcode::LSTORE_0: frame.storeLongValue(0, frame.popLong()); break;
      case Opcode::LSTORE_1: frame.storeLongValue(1, frame.popLong()); break;
      case Opcode::LSTORE_2: frame.storeLongValue(2, frame.popLong()); break;
      case Opcode::LSTORE_3: frame.storeLongValue(3, frame.popLong()); break;
      case Opcode::FSTORE_0: notImplemented(opcode); break;
      case Opcode::FSTORE_1: notImplemented(opcode); break;
      case Opcode::FSTORE_2: notImplemented(opcode); break;
      case Opcode::FSTORE_3: notImplemented(opcode); break;
      case Opcode::DSTORE_0: notImplemented(opcode); break;
      case Opcode::DSTORE_1: notImplemented(opcode); break;
      case Opcode::DSTORE_2: notImplemented(opcode); break;
      case Opcode::DSTORE_3: notImplemented(opcode); break;
      case Opcode::ASTORE_0: frame.storeValue(0, frame.popOperand()); break;
      case Opcode::ASTORE_1: frame.storeValue(1, frame.popOperand()); break;
      case Opcode::ASTORE_2: frame.storeValue(2, frame.popOperand()); break;
      case Opcode::ASTORE_3: frame.storeValue(3, frame.popOperand()); break;
      case Opcode::IASTORE: {
        Value value = frame.popOperand();
        int32_t index = frame.popOperand().asInt();
        ArrayInstance* arrayRef = frame.popOperand().asReference()->asArrayInstance();

        arrayRef->setArrayElement(index, value);
        // TODO: null-check, bounds check, type checks

        break;
      }
      case Opcode::LASTORE: notImplemented(opcode); break;
      case Opcode::FASTORE: notImplemented(opcode); break;
      case Opcode::DASTORE: notImplemented(opcode); break;
      case Opcode::AASTORE: {
        Value value = frame.popOperand();
        int32_t index = frame.popOperand().asInt();
        ArrayInstance* arrayRef = frame.popOperand().asReference()->asArrayInstance();

        arrayRef->setArrayElement(index, value);

        // TODO: null-check, bounds check, type checks
        break;
      }
      case Opcode::BASTORE: notImplemented(opcode); break;
      case Opcode::CASTORE: {
        int32_t value = frame.popOperand().asInt();
        int32_t index = frame.popOperand().asInt();
        ArrayInstance* arrayRef = frame.popOperand().asReference()->asArrayInstance();

        // FIXME: Explicitly truncate
        // TODO: Null check, exceptions
        arrayRef->setArrayElement(index, Value::Char(static_cast<char16_t>(value)));

        break;
      }
      case Opcode::SASTORE: notImplemented(opcode); break;
      case Opcode::POP: {
        frame.popOperand();
        break;
      }
      case Opcode::POP2: notImplemented(opcode); break;
      case Opcode::DUP: {
        // TOOD: Duplicate instead of pop / push
        auto value = frame.popOperand();
        frame.pushOperand(value);
        frame.pushOperand(value);
        break;
      }
      case Opcode::DUP_X1: {
        Value value1 = frame.popOperand();
        Value value2 = frame.popOperand();

        frame.pushOperand(value1);
        frame.pushOperand(value2);
        frame.pushOperand(value1);

        break;
      }
      case Opcode::DUP_X2: notImplemented(opcode); break;
      case Opcode::DUP2: {
        // Category 1
        Value value1 = frame.popOperand();
        if (value1.isCategoryTwo()) {
          frame.pushOperand(value1);
          frame.pushOperand(value1);
        } else {
          Value value2 = frame.popOperand();
          frame.pushOperand(value2);
          frame.pushOperand(value1);
          frame.pushOperand(value2);
          frame.pushOperand(value1);
        }

        break;
      }
      case Opcode::DUP2_X1: notImplemented(opcode); break;
      case Opcode::DUP2_X2: notImplemented(opcode); break;
      case Opcode::SWAP: notImplemented(opcode); break;
      case Opcode::IADD: {
        int32_t value2 = frame.popOperand().asInt();
        int32_t value1 = frame.popOperand().asInt();

        int32_t result = value2 + value1;

        frame.pushOperand(Value::Int(result));
        break;
      }
      case Opcode::LADD: {
        int64_t value2 = frame.popOperand().asLong();
        int64_t value1 = frame.popOperand().asLong();

        frame.pushOperand(Value::Long(value1 + value2));
        break;
      }
      case Opcode::FADD: notImplemented(opcode); break;
      case Opcode::DADD: notImplemented(opcode); break;
      case Opcode::ISUB: {
        int32_t value2 = frame.popOperand().asInt();
        int32_t value1 = frame.popOperand().asInt();

        int32_t result = value1 - value2;

        frame.pushOperand(Value::Int(result));
        break;
      }
      case Opcode::LSUB: notImplemented(opcode); break;
      case Opcode::FSUB: notImplemented(opcode); break;
      case Opcode::DSUB: notImplemented(opcode); break;
      case Opcode::IMUL: {
        int32_t value2 = frame.popOperand().asInt();
        int32_t value1 = frame.popOperand().asInt();
        frame.pushOperand(Value::Int(value1 * value2));

        break;
      }
      case Opcode::LMUL: notImplemented(opcode); break;
      case Opcode::FMUL: {
        float value2 = frame.popOperand().asFloat();
        float value1 = frame.popOperand().asFloat();
        frame.pushOperand(Value::Float(value1 * value2));

        break;
      }
      case Opcode::DMUL: notImplemented(opcode); break;
      case Opcode::IDIV: notImplemented(opcode); break;
      case Opcode::LDIV: notImplemented(opcode); break;
      case Opcode::FDIV: notImplemented(opcode); break;
      case Opcode::DDIV: notImplemented(opcode); break;
      case Opcode::IREM: {
        int32_t value2 = frame.popOperand().asInt();
        int32_t value1 = frame.popOperand().asInt();

        int32_t result = value1 - (value1 / value2) * value2;

        frame.pushOperand(Value::Int(result));
        break;
      }
      case Opcode::LREM: {
        int64_t value2 = frame.popOperand().asLong();
        int64_t value1 = frame.popOperand().asLong();

        int64_t result = value1 - (value1 / value2) * value2;

        frame.pushOperand(Value::Long(result));
        break;
      }
      case Opcode::FREM: notImplemented(opcode); break;
      case Opcode::DREM: notImplemented(opcode); break;
      case Opcode::INEG: notImplemented(opcode); break;
      case Opcode::LNEG: notImplemented(opcode); break;
      case Opcode::FNEG: notImplemented(opcode); break;
      case Opcode::DNEG: notImplemented(opcode); break;
      case Opcode::ISHL: {
        int32_t value2 = frame.popOperand().asInt();
        int32_t value1 = frame.popOperand().asInt();

        uint32_t offset = std::bit_cast<uint32_t>(value2) & 0x0000001F;

        frame.pushOperand(Value::Int(value1 << offset));

        break;
      }
      case Opcode::LSHL: {
        int32_t value2 = frame.popOperand().asInt();
        int64_t value1 = frame.popOperand().asLong();

        uint32_t offset = std::bit_cast<uint32_t>(value2) & 0x0000003F;

        frame.pushOperand(Value::Long(value1 << offset));

        break;
      }
      case Opcode::ISHR: notImplemented(opcode); break;
      case Opcode::LSHR: notImplemented(opcode); break;
      case Opcode::IUSHR: {
        int32_t value2 = frame.popOperand().asInt();
        int32_t value1 = frame.popOperand().asInt();

        uint32_t offset = std::bit_cast<uint32_t>(value2) & 0x0000001F;

        // TODO: Is this working according to spec?
        frame.pushOperand(Value::Int(value1 >> offset));
        break;
      }
      case Opcode::LUSHR: notImplemented(opcode); break;
      case Opcode::IAND: {
        int32_t value2 = frame.popOperand().asInt();
        int32_t value1 = frame.popOperand().asInt();

        frame.pushOperand(Value::Int(value1 & value2));
        break;
      }
      case Opcode::LAND: {
        int64_t value2 = frame.popOperand().asLong();
        int64_t value1 = frame.popOperand().asLong();

        frame.pushOperand(Value::Long(value1 & value2));
        break;
      }
      case Opcode::IOR: notImplemented(opcode); break;
      case Opcode::LOR: notImplemented(opcode); break;
      case Opcode::IXOR: {
        int32_t value2 = frame.popOperand().asInt();
        int32_t value1 = frame.popOperand().asInt();

        frame.pushOperand(Value::Int(value1 ^ value2));

        break;
      }
      case Opcode::LXOR: notImplemented(opcode); break;
      case Opcode::IINC: {
        types::u1 index = cursor.readU1();
        auto constValue = static_cast<int32_t>(std::bit_cast<int8_t>(cursor.readU1()));

        frame.storeValue(index, Value::Int(frame.loadValue(index).asInt() + constValue));

        break;
      }
      case Opcode::I2L: {
        int32_t value = frame.popOperand().asInt();
        frame.pushOperand(Value::Long(static_cast<int64_t>(value)));
        break;
      }
      case Opcode::I2F: {
        int32_t value = frame.popOperand().asInt();
        frame.pushOperand(Value::Float(static_cast<float>(value)));
        break;
      }
      case Opcode::I2D: notImplemented(opcode); break;
      case Opcode::L2I: notImplemented(opcode); break;
      case Opcode::L2F: notImplemented(opcode); break;
      case Opcode::L2D: notImplemented(opcode); break;
      case Opcode::F2I: {
        // TODO: Is this ok according to spec?
        float value = frame.popOperand().asFloat();
        frame.pushOperand(Value::Int(static_cast<int32_t>(value)));
        break;
      }
      case Opcode::F2L: notImplemented(opcode); break;
      case Opcode::F2D: notImplemented(opcode); break;
      case Opcode::D2I: notImplemented(opcode); break;
      case Opcode::D2L: notImplemented(opcode); break;
      case Opcode::D2F: notImplemented(opcode); break;
      case Opcode::I2B: notImplemented(opcode); break;
      case Opcode::I2C: notImplemented(opcode); break;
      case Opcode::I2S: notImplemented(opcode); break;

      case Opcode::LCMP: {
        int64_t value2 = frame.popOperand().asLong();
        int64_t value1 = frame.popOperand().asLong();

        if (value1 > value2) {
          frame.pushOperand(Value::Int(1));
        } else if (value1 < value2) {
          frame.pushOperand(Value::Int(-1));
        } else {
          frame.pushOperand(Value::Int(0));
        }

        break;
      }
      case Opcode::FCMPL: {
        float value2 = frame.popOperand().asFloat();
        float value1 = frame.popOperand().asFloat();

        if (value1 > value2) {
          frame.pushOperand(Value::Int(1));
        } else if (value1 == value2) {
          // TODO: Is this IEE equality?
          frame.pushOperand(Value::Int(0));
        } else if (value1 < value2) {
          frame.pushOperand(Value::Int(-1));
        } else {
          frame.pushOperand(Value::Int(-1));
        }
        break;
      }
      case Opcode::FCMPG: {
        float value2 = frame.popOperand().asFloat();
        float value1 = frame.popOperand().asFloat();

        if (value1 > value2) {
          frame.pushOperand(Value::Int(1));
        } else if (value1 == value2) {
          // TODO: Is this IEE equality?
          frame.pushOperand(Value::Int(0));
        } else if (value1 < value2) {
          frame.pushOperand(Value::Int(-1));
        } else {
          frame.pushOperand(Value::Int(1));
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

        Instance* value2 = frame.popOperand().asReference();
        Instance* value1 = frame.popOperand().asReference();

        auto offset = std::bit_cast<int16_t>(cursor.readU2());

        if (value1 == value2) {
          cursor.set(opcodePos + offset);
        }

        break;
      }
      case Opcode::IF_ACMPNE: {
        auto opcodePos = cursor.position() - 1;

        Instance* value2 = frame.popOperand().asReference();
        Instance* value1 = frame.popOperand().asReference();

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
      case Opcode::IRETURN: return Value::Int(frame.popOperand().asInt());
      case Opcode::LRETURN: return Value::Long(frame.popOperand().asLong());
      case Opcode::FRETURN: return Value::Float(frame.popOperand().asFloat());
      case Opcode::DRETURN: return Value::Double(frame.popOperand().asDouble());
      case Opcode::ARETURN: return Value::Reference(frame.popOperand().asReference());
      case Opcode::RETURN: return std::nullopt;
      case Opcode::GETSTATIC: {
        auto index = cursor.readU2();
        auto& fieldRef = runtimeConstantPool.getFieldRef(index);

        JClass* klass = fieldRef.klass;
        klass->initialize(thread);

        Value value = klass->getStaticField(fieldRef.fieldName);
        frame.pushOperand(value);

        break;
      }
      case Opcode::PUTSTATIC: {
        auto index = cursor.readU2();
        auto& fieldRef = runtimeConstantPool.getFieldRef(index);

        JClass* klass = fieldRef.klass;
        klass->initialize(thread);
        klass->storeStaticField(fieldRef.fieldName, frame.popOperand());

        break;
      }
      case Opcode::GETFIELD: {
        types::u2 index = cursor.readU2();
        auto field = runtimeConstantPool.getFieldRef(index);
        Instance* objectRef = frame.popOperand().asReference();

        // TODO: Null check

        assert(objectRef->getClass()->isInstanceOf(field.klass));
        frame.pushOperand(objectRef->getFieldValue(field.fieldName, field.fieldDescriptor));
        break;
      }
      case Opcode::PUTFIELD: {
        auto index = cursor.readU2();
        auto field = runtimeConstantPool.getFieldRef(index);

        Value value = frame.popOperand();
        Instance* objectRef = frame.popOperand().asReference();
        assert(objectRef->getClass()->isInstanceOf(field.klass));

        objectRef->setFieldValue(field.fieldName, field.fieldDescriptor, value);

        break;
      }
      case Opcode::INVOKEVIRTUAL: {
        auto index = cursor.readU2();
        const JMethod* baseMethod = runtimeConstantPool.getMethodRef(index);

        int numArgs = baseMethod->descriptor().parameters().size();
        Value objectRef = frame.peek(numArgs);
        if (objectRef.asReference() == nullptr) {
          thread.throwException(u"java/lang/NullPointerException");
          break;
        }

        JClass* target = objectRef.asReference()->getClass();
        auto targetMethod = target->getVirtualMethod(baseMethod->name(), baseMethod->rawDescriptor());

        assert(targetMethod.has_value());

        this->invoke(thread, *targetMethod);

        break;
      }
      case Opcode::INVOKESPECIAL: {
        auto index = cursor.readU2();
        JMethod* method = runtimeConstantPool.getMethodRef(index);

        this->invoke(thread, method);

        break;
      }
      case Opcode::INVOKESTATIC: {
        auto index = cursor.readU2();
        JMethod* method = runtimeConstantPool.getMethodRef(index);
        assert(method->isStatic());

        method->getClass()->initialize(thread);

        this->invoke(thread, method);

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

        JClass* target = objectRef.asReference()->getClass();
        auto method = target->getVirtualMethod(methodRef->name(), methodRef->rawDescriptor());

        this->invoke(thread, *method);
        break;
      }
      case Opcode::INVOKEDYNAMIC: notImplemented(opcode); break;
      case Opcode::NEW: {
        auto index = cursor.readU2();
        auto className = frame.currentClass()->constantPool().getClassName(index);

        auto klass = thread.resolveClass(types::JString{className});
        if (!klass) {
          // TODO: Abort frame
          assert(false);
        }

        (*klass)->initialize(thread);

        if (auto instanceClass = (*klass)->asInstanceClass(); instanceClass != nullptr) {
          Instance* instance = thread.heap().allocate(instanceClass);
          frame.pushOperand(Value::Reference(instance));
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
        int32_t count = frame.popOperand().asInt();

        types::JString arrayClsName;

        switch (atype) {
          case ArrayType::T_BOOLEAN: arrayClsName = u"[Z"; break;
          case ArrayType::T_CHAR: arrayClsName = u"[C"; break;
          case ArrayType::T_FLOAT: arrayClsName = u"[F"; break;
          case ArrayType::T_DOUBLE: arrayClsName = u"[D"; break;
          case ArrayType::T_BYTE: arrayClsName = u"[B"; break;
          case ArrayType::T_SHORT: arrayClsName = u"[S"; break;
          case ArrayType::T_INT: arrayClsName = u"[I"; break;
          case ArrayType::T_LONG: arrayClsName = u"[L"; break;
          default: assert(false && "impossible"); break;
        }

        auto arrayClass = thread.resolveClass(arrayClsName);
        if (!arrayClass) {
          // TODO: Abort frame
          assert(false);
        }

        ArrayInstance* newInstance = thread.heap().allocateArray((*arrayClass)->asArrayClass(), count);
        frame.pushOperand(Value::Reference(newInstance));

        break;
      }
      case Opcode::ANEWARRAY: {
        auto index = cursor.readU2();
        int32_t count = frame.popOperand().asInt();

        auto klass = runtimeConstantPool.getClass(index);
        if (!klass) {
          // TODO: Abort frame
          assert(false);
        }

        auto arrayClass = thread.resolveClass(u"[L" + types::JString{(*klass)->className()} + u";");
        assert(arrayClass.has_value());

        ArrayInstance* array = thread.heap().allocateArray((*arrayClass)->asArrayClass(), count);
        frame.pushOperand(Value::Reference(array));
        // TODO: if count is less than zero, the anewarray instruction throws a NegativeArraySizeException.

        break;
      }
      case Opcode::ARRAYLENGTH: {
        ArrayInstance* arrayRef = frame.popOperand().asReference()->asArrayInstance();
        frame.pushOperand(Value::Int(arrayRef->length()));
        break;
      }
      case Opcode::ATHROW: {
        auto exception = frame.popOperand().asReference();
        thread.throwException(exception);
        break;
      }
      case Opcode::CHECKCAST: {
        types::u2 index = cursor.readU2();
        auto objectRef = frame.popOperand().asReference();
        if (objectRef == nullptr) {
          frame.pushOperand(Value::Reference(objectRef));
        } else {
          auto klass = runtimeConstantPool.getClass(index);
          if (!klass) {
            // TODO: Abort frame
            assert(false);
          }

          JClass* classToCheck = objectRef->getClass();
          if (!classToCheck->isInstanceOf(*klass)) {
            assert(false && "TODO throw execption");
          } else {
            frame.pushOperand(Value::Reference(objectRef));
          }
        }

        break;
      }
      case Opcode::INSTANCEOF: {
        auto index = cursor.readU2();
        Instance* objectRef = frame.popOperand().asReference();
        if (objectRef == nullptr) {
          frame.pushOperand(Value::Int(0));
        } else {
          auto klass = runtimeConstantPool.getClass(index);
          if (!klass) {
            // TODO: Abort frame
            assert(false);
          }

          JClass* classToCheck = objectRef->getClass();
          if (classToCheck->isInstanceOf(*klass)) {
            frame.pushOperand(Value::Int(1));
          } else {
            frame.pushOperand(Value::Int(0));
          }
        }

        break;
      }
      case Opcode::MONITORENTER: {
        // FIXME
        frame.popOperand();
        break;
      }
      case Opcode::MONITOREXIT: {
        // FIXME
        frame.popOperand();
        break;
      }

      case Opcode::WIDE: notImplemented(opcode); break;
      case Opcode::MULTIANEWARRAY: notImplemented(opcode); break;
      case Opcode::IFNULL: {
        auto opcodePos = cursor.position() - 1;
        auto offset = std::bit_cast<int16_t>(cursor.readU2());

        Instance* value = frame.popOperand().asReference();
        if (value == nullptr) {
          cursor.set(opcodePos + offset);
        }

        break;
      }
      case Opcode::IFNONNULL: {
        auto opcodePos = cursor.position() - 1;
        auto offset = std::bit_cast<int16_t>(cursor.readU2());

        Instance* value = frame.popOperand().asReference();
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
    if (auto exception = thread.currentException(); exception != nullptr) {
      size_t pc = cursor.position() - 1;

      bool handled = false;
      for (auto& entry : code.exceptionTable()) {
        if (pc < entry.startPc || pc >= entry.endPc) {
          // The exception handler is not active
          continue;
        }

        bool caught = false;
        if (entry.catchType != 0) {
          auto exceptionClass = runtimeConstantPool.getClass(entry.catchType);
          assert(exceptionClass.has_value());
          if (exception->getClass()->isInstanceOf(*exceptionClass)) {
            caught = true;
          }
        } else {
          caught = true;
        }

        if (caught) {
          handled = true;
          cursor.set(entry.handlerPc);
          break;
        }
      }

      if (!handled) {
        return std::nullopt;
      } else {
        thread.clearException();
      }
    }
  }

  assert(false && "Should be unreachable");
}

static bool compareInt(Predicate predicate, Value val1, Value val2)
{
  switch (predicate) {
    case Predicate::Eq: return val1.asInt() == val2.asInt();
    case Predicate::NotEq: return val1.asInt() != val2.asInt();
    case Predicate::Gt: return val1.asInt() > val2.asInt();
    case Predicate::Lt: return val1.asInt() < val2.asInt();
    case Predicate::GtEq: return val1.asInt() >= val2.asInt();
    case Predicate::LtEq: return val1.asInt() <= val2.asInt();
  }

  std::unreachable();
}

void DefaultInterpreter::invoke(JavaThread& thread, JMethod* method)
{
  auto returnValue = thread.invoke(method);
  assert((method->isVoid() || thread.currentException() != nullptr) || returnValue.has_value());

  if (returnValue.has_value()) {
    thread.currentFrame().pushOperand(*returnValue);
  }
}

void DefaultInterpreter::integerComparison(Predicate predicate, CodeCursor& cursor, CallFrame& frame)
{
  auto opcodePos = cursor.position() - 1;

  auto val2 = frame.popInt();
  auto val1 = frame.popInt();

  auto offset = std::bit_cast<int16_t>(cursor.readU2());

  if (compareInt(predicate, val1, val2)) {
    cursor.set(opcodePos + offset);
  }
}

void DefaultInterpreter::integerComparisonToZero(Predicate predicate, CodeCursor& cursor, CallFrame& frame)
{
  auto opcodePos = cursor.position() - 1;

  auto val1 = frame.popInt();

  auto offset = std::bit_cast<int16_t>(cursor.readU2());

  if (compareInt(predicate, val1, Value::Int(0))) {
    cursor.set(opcodePos + offset);
  }
}

#include "vm/Interpreter.h"

#include <iostream>

#include "class_file/Code.h"
#include "class_file/Opcode.h"
#include "vm/Frame.h"
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
  void execute(Vm& vm, const Code& code, std::size_t pc) override;

private:
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

void DefaultInterpreter::execute(Vm& vm, const Code& code, std::size_t startPc)
{
  CodeCursor cursor(code.bytes(), startPc);

  while (cursor.hasNext()) {
    Opcode opcode = cursor.next();
    CallFrame& frame = vm.currentFrame();
    RuntimeConstantPool& runtimeConstantPool = frame.currentClass()->runtimeConstantPool();

    // std::cout << "#" << cursor.position() << " " << opcodeToString(opcode) << std::endl;
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
      case Opcode::FCONST_0: notImplemented(opcode); break;
      case Opcode::FCONST_1: notImplemented(opcode); break;
      case Opcode::FCONST_2: notImplemented(opcode); break;
      case Opcode::DCONST_0: notImplemented(opcode); break;
      case Opcode::DCONST_1: notImplemented(opcode); break;
      case Opcode::BIPUSH: {
        auto byte = std::bit_cast<int8_t>(cursor.readU1());
        frame.pushOperand(Value::Int(static_cast<int32_t>(byte)));
        break;
      }
      case Opcode::SIPUSH: notImplemented(opcode); break;
      case Opcode::LDC: {
        types::u1 index = cursor.readU1();
        auto entry = frame.currentClass()->constantPool().getEntry(index);

        if (entry.tag == ConstantPool::Tag::CONSTANT_Integer) {
          frame.pushOperand(Value::Int(entry.data.singleInteger));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_Float) {
          frame.pushOperand(Value::Float(entry.data.singleFloat));
        } else if (entry.tag == ConstantPool::Tag::CONSTANT_String) {
          frame.pushOperand(Value::Reference(runtimeConstantPool.getString(index)));
        }
        break;
      }
      case Opcode::LDC_W: notImplemented(opcode); break;
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
      case Opcode::LLOAD: notImplemented(opcode); break;
      case Opcode::FLOAD: notImplemented(opcode); break;
      case Opcode::DLOAD: notImplemented(opcode); break;
      case Opcode::ALOAD: notImplemented(opcode); break;
      case Opcode::ILOAD_0: frame.pushOperand(frame.loadValue(0)); break;
      case Opcode::ILOAD_1: frame.pushOperand(frame.loadValue(1)); break;
      case Opcode::ILOAD_2: frame.pushOperand(frame.loadValue(2)); break;
      case Opcode::ILOAD_3: frame.pushOperand(frame.loadValue(3)); break;
      case Opcode::LLOAD_0: frame.pushOperand(frame.loadValue(0)); break;
      case Opcode::LLOAD_1: frame.pushOperand(frame.loadValue(1)); break;
      case Opcode::LLOAD_2: frame.pushOperand(frame.loadValue(2)); break;
      case Opcode::LLOAD_3: frame.pushOperand(frame.loadValue(3)); break;
      case Opcode::FLOAD_0: notImplemented(opcode); break;
      case Opcode::FLOAD_1: notImplemented(opcode); break;
      case Opcode::FLOAD_2: notImplemented(opcode); break;
      case Opcode::FLOAD_3: notImplemented(opcode); break;
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
      case Opcode::AALOAD: notImplemented(opcode); break;
      case Opcode::BALOAD: notImplemented(opcode); break;
      case Opcode::CALOAD: notImplemented(opcode); break;
      case Opcode::SALOAD: notImplemented(opcode); break;
      case Opcode::ISTORE: {
        auto index = cursor.readU1();
        frame.storeValue(index, frame.popInt());
        break;
      }
      case Opcode::LSTORE: notImplemented(opcode); break;
      case Opcode::FSTORE: notImplemented(opcode); break;
      case Opcode::DSTORE: notImplemented(opcode); break;
      case Opcode::ASTORE: notImplemented(opcode); break;
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
      case Opcode::IASTORE: notImplemented(opcode); break;
      case Opcode::LASTORE: notImplemented(opcode); break;
      case Opcode::FASTORE: notImplemented(opcode); break;
      case Opcode::DASTORE: notImplemented(opcode); break;
      case Opcode::AASTORE: notImplemented(opcode); break;
      case Opcode::BASTORE: notImplemented(opcode); break;
      case Opcode::CASTORE: notImplemented(opcode); break;
      case Opcode::SASTORE: notImplemented(opcode); break;
      case Opcode::POP: notImplemented(opcode); break;
      case Opcode::POP2: notImplemented(opcode); break;
      case Opcode::DUP: {
        // TOOD: Duplicate instead of pop / push
        auto value = frame.popOperand();
        frame.pushOperand(value);
        frame.pushOperand(value);
        break;
      }
      case Opcode::DUP_X1: notImplemented(opcode); break;
      case Opcode::DUP_X2: notImplemented(opcode); break;
      case Opcode::DUP2: notImplemented(opcode); break;
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
      case Opcode::ISUB: notImplemented(opcode); break;
      case Opcode::LSUB: notImplemented(opcode); break;
      case Opcode::FSUB: notImplemented(opcode); break;
      case Opcode::DSUB: notImplemented(opcode); break;
      case Opcode::IMUL: notImplemented(opcode); break;
      case Opcode::LMUL: notImplemented(opcode); break;
      case Opcode::FMUL: notImplemented(opcode); break;
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
      case Opcode::ISHL: notImplemented(opcode); break;
      case Opcode::LSHL: notImplemented(opcode); break;
      case Opcode::ISHR: notImplemented(opcode); break;
      case Opcode::LSHR: notImplemented(opcode); break;
      case Opcode::IUSHR: notImplemented(opcode); break;
      case Opcode::LUSHR: notImplemented(opcode); break;
      case Opcode::IAND: notImplemented(opcode); break;
      case Opcode::LAND: notImplemented(opcode); break;
      case Opcode::IOR: notImplemented(opcode); break;
      case Opcode::LOR: notImplemented(opcode); break;
      case Opcode::IXOR: notImplemented(opcode); break;
      case Opcode::LXOR: notImplemented(opcode); break;
      case Opcode::IINC: {
        types::u1 index = cursor.readU1();
        auto constValue = static_cast<int32_t>(std::bit_cast<int8_t>(cursor.readU1()));

        frame.storeValue(index, Value::Int(frame.loadValue(index).asInt() + constValue));

        break;
      }
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
      case Opcode::FCMPL: notImplemented(opcode); break;
      case Opcode::FCMPG: notImplemented(opcode); break;
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
      case Opcode::IF_ACMPEQ: notImplemented(opcode); break;
      case Opcode::IF_ACMPNE: notImplemented(opcode); break;
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
      case Opcode::IRETURN: {
        vm.returnToCaller(frame.popOperand());
        return;
      }
      case Opcode::LRETURN: notImplemented(opcode); break;
      case Opcode::FRETURN: notImplemented(opcode); break;
      case Opcode::DRETURN: notImplemented(opcode); break;
      case Opcode::ARETURN: notImplemented(opcode); break;
      case Opcode::RETURN: vm.returnToCaller(); return;
      case Opcode::GETSTATIC: {
        auto index = cursor.readU2();
        auto& fieldRef = runtimeConstantPool.getFieldRef(index);

        auto klass = vm.resolveClass(fieldRef.className);
        if (!klass) {
          vm.raiseError(*klass.error());
          // TODO: Abort frame
        }

        Value value = (*klass)->getStaticField(fieldRef.fieldName);
        frame.pushOperand(value);

        break;
      }
      case Opcode::PUTSTATIC: {
        auto index = cursor.readU2();
        auto& fieldRef = runtimeConstantPool.getFieldRef(index);

        auto klass = vm.resolveClass(fieldRef.className);
        if (!klass) {
          vm.raiseError(*klass.error());
          // TODO: Abort frame
        }

        (*klass)->storeStaticField(fieldRef.fieldName, frame.popOperand());
        break;
      }
      case Opcode::GETFIELD: notImplemented(opcode); break;
      case Opcode::PUTFIELD: notImplemented(opcode); break;
      case Opcode::INVOKEVIRTUAL: {
        auto index = cursor.readU2();
        auto methodRef = runtimeConstantPool.getMethodRef(index);

        auto klass = vm.resolveClass(methodRef.className);
        if (!klass) {
          // TODO: Abort frame
          vm.raiseError(*klass.error());
        }

        JMethod* baseMethod = (*klass)->getMethod(methodRef.methodName, methodRef.methodDescriptor);

        int numArgs = baseMethod->getDescriptor().parameters().size();
        Value objectRef = frame.peek(numArgs);

        JClass* target = objectRef.asReference()->getClass();
        JMethod* method = target->getMethod(methodRef.methodName, methodRef.methodDescriptor);
        while (method == nullptr) {
          if (auto parentName = target->superClass(); parentName.has_value()) {
            target = *vm.resolveClass(types::JString{*parentName});
            method = target->getMethod(methodRef.methodName, methodRef.methodDescriptor);
          } else {
            assert(false && "TODO: InvokeVirtual");
          }
        }

        vm.invoke(target, method);
        // TODO: signature method

        break;
      }
      case Opcode::INVOKESPECIAL: {
        auto index = cursor.readU2();
        auto& methodRef = runtimeConstantPool.getMethodRef(index);

        auto klass = vm.resolveClass(methodRef.className);
        if (!klass) {
          // TODO: Abort frame
          vm.raiseError(*klass.error());
        }

        auto method = vm.resolveMethod(*klass, methodRef.methodName, methodRef.methodDescriptor);
        vm.invoke(*klass, method);

        break;
      }
      case Opcode::INVOKESTATIC: {
        auto index = cursor.readU2();
        auto& methodRef = runtimeConstantPool.getMethodRef(index);

        auto klass = vm.resolveClass(methodRef.className);
        if (!klass) {
          vm.raiseError(*klass.error());
          // TODO: Abort frame
        }

        auto method = vm.resolveStaticMethod(*klass, methodRef.methodName, methodRef.methodDescriptor);
        vm.invoke(*klass, method);

        break;
      }
      case Opcode::INVOKEINTERFACE: notImplemented(opcode); break;
      case Opcode::INVOKEDYNAMIC: notImplemented(opcode); break;
      case Opcode::NEW: {
        auto index = cursor.readU2();
        auto className = frame.currentClass()->constantPool().getClassName(index);

        auto klass = vm.resolveClass(types::JString{className});
        if (!klass) {
          vm.raiseError(*klass.error());
          // TODO: Abort frame
        }

        Instance* instance = vm.newInstance(*klass);
        frame.pushOperand(Value::Reference(instance));

        break;
      }
      case Opcode::NEWARRAY: notImplemented(opcode); break;
      case Opcode::ANEWARRAY: {
        auto index = cursor.readU2();
        int32_t count = frame.popOperand().asInt();

        auto klass = runtimeConstantPool.getClass(index);
        if (!klass) {
          vm.raiseError(*klass.error());
        }

        auto arrayClass = vm.resolveArrayClass(u"[L" + types::JString{(*klass)->getName()} + u";");
        assert(arrayClass.has_value());

        ArrayInstance* array = vm.newArrayInstance(*arrayClass, count);
        frame.pushOperand(Value::Reference(array));
        // TODO: if count is less than zero, the anewarray instruction throws a NegativeArraySizeException.

        break;
      }
      case Opcode::ARRAYLENGTH: notImplemented(opcode); break;
      case Opcode::ATHROW: notImplemented(opcode); break;
      case Opcode::CHECKCAST: notImplemented(opcode); break;
      case Opcode::INSTANCEOF: notImplemented(opcode); break;
      case Opcode::MONITORENTER: notImplemented(opcode); break;
      case Opcode::MONITOREXIT: notImplemented(opcode); break;
      case Opcode::WIDE: notImplemented(opcode); break;
      case Opcode::MULTIANEWARRAY: notImplemented(opcode); break;
      case Opcode::IFNULL: notImplemented(opcode); break;
      case Opcode::IFNONNULL: notImplemented(opcode); break;
      case Opcode::GOTO_W: notImplemented(opcode); break;
      case Opcode::JSR_W: notImplemented(opcode); break;
      case Opcode::BREAKPOINT: notImplemented(opcode); break;
      case Opcode::IMPDEP1: notImplemented(opcode); break;
      case Opcode::IMPDEP2: notImplemented(opcode); break;
    }
  }
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

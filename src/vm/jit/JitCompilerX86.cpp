#include "common/ByteStream.h"
#include "vm/Vm.h"
#include "vm/jit/JitCompiler.h"

#include <asmjit/core/jitruntime.h>
#include <asmjit/core/logger.h>
#include <asmjit/x86/x86compiler.h>
#include <class_file/Opcode.h>

using namespace geevm;

namespace
{

class AsmJitDebugLogger : public asmjit::Logger
{
public:
  asmjit::Error _log(const char* data, size_t size) noexcept override
  {
    debug::DebugLogger::get().log("JITx86", data);
    return asmjit::kErrorOk;
  }
};

class AsmJitErrorHandler : public asmjit::ErrorHandler
{
public:
  void handleError(asmjit::Error err, const char* message, asmjit::BaseEmitter* origin) override
  {
    debug::DebugLogger::get().log("JIT x86", std::format("AsmJit error: {}\n", message));
  }
};

class JitCompilerX86Impl
{
public:
  explicit JitCompilerX86Impl(JMethod* method, asmjit::CodeHolder* codeHolder);

  void doCompile();

private:
  template<std::derived_from<asmjit::Operand> T>
  void push(const T& value)
  {
    auto& target = mStack[mStackPointer++];
    mCompiler.mov(target, value);
  }

  template<std::derived_from<asmjit::Operand> T>
  void pushCategoryTwo(const T& value)
  {
    auto& target = mStack[mStackPointer++];
    mStackPointer++;
    mCompiler.mov(target, value);
  }

  asmjit::x86::Gp& pop()
  {
    mStackPointer--;
    return mStack[mStackPointer];
  }

  asmjit::x86::Gp& popCategoryTwo()
  {
    mStackPointer -= 2;
    return mStack[mStackPointer];
  }

  void binaryIntOp(const std::function<void(asmjit::x86::Gp&, asmjit::x86::Gp&)>& function)
  {
    auto& value2 = this->pop();
    auto& value1 = this->pop();

    function(value1, value2);
    mStackPointer++;
  }

private:
  JMethod* mMethod;
  asmjit::CodeHolder* mCode;
  asmjit::x86::Compiler mCompiler;
  ByteStream mBytes;
  asmjit::FuncNode* mFunction = nullptr;

  std::vector<asmjit::x86::Gp> mStack;
  size_t mStackPointer = 0;
  asmjit::x86::Gp mLocals;
};

class JitCompilerX86 : public JitCompiler
{
public:
  explicit JitCompilerX86(Vm& vm)
    : mVm(vm)
  {
  }

  JitFunction compile(JMethod* method) override;

private:
  Vm& mVm;
  asmjit::JitRuntime mJitRuntime;
};

} // namespace

std::unique_ptr<JitCompiler> JitCompiler::create(Vm& vm)
{
  return std::make_unique<JitCompilerX86>(vm);
}

JitCompilerX86Impl::JitCompilerX86Impl(JMethod* method, asmjit::CodeHolder* code)
  : mMethod(method), mCode(code), mCompiler(mCode), mBytes(method->getCode().bytes())
{
  mCompiler.setLogger(mCode->logger());
  mCompiler.setErrorHandler(mCode->errorHandler());

  mFunction = mCompiler.addFunc(asmjit::FuncSignature::build<uint64_t, uint64_t*>());

  mLocals = mCompiler.newIntPtr();
  mFunction->setArg(0, mLocals);

  for (size_t i = 0; i < mMethod->getCode().maxStack(); i++) {
    mStack.emplace_back(mCompiler.newGpq());
  }
}

static void notImplemented(Opcode opcode)
{
  geevm_panic(std::format("cannot jit-compile unsupported opcode '{}'", opcodeToString(opcode)));
}

JitFunction JitCompilerX86::compile(JMethod* method)
{
  AsmJitDebugLogger logger;
  logger.addFlags(asmjit::FormatFlags::kMachineCode);
  logger.addFlags(asmjit::FormatFlags::kHexOffsets);
  logger.addFlags(asmjit::FormatFlags::kExplainImms);
  logger.addFlags(asmjit::FormatFlags::kHexImms);
  logger.addFlags(asmjit::FormatFlags::kPositions);
  logger.addFlags(asmjit::FormatFlags::kRegCasts);
  logger.addFlags(asmjit::FormatFlags::kRegType);

  AsmJitErrorHandler errorHandler;

  asmjit::CodeHolder code;
  code.setLogger(&logger);
  code.init(mJitRuntime.environment(), mJitRuntime.cpuFeatures());
  code.setErrorHandler(&errorHandler);

  JitCompilerX86Impl impl{method, &code};
  impl.doCompile();

  JitFunction fnPtr = nullptr;
  asmjit::Error err = mJitRuntime.add(&fnPtr, &code);
  if (err) {
    geevm_panic("Failed to JIT compile");
  }

  return fnPtr;
}

void JitCompilerX86Impl::doCompile()
{
  using namespace asmjit;

  AsmJitDebugLogger logger;
  logger.addFlags(FormatFlags::kMachineCode);
  logger.addFlags(FormatFlags::kHexOffsets);
  logger.addFlags(FormatFlags::kExplainImms);
  logger.addFlags(FormatFlags::kHexImms);
  logger.addFlags(FormatFlags::kPositions);
  logger.addFlags(FormatFlags::kRegCasts);
  logger.addFlags(FormatFlags::kRegType);

  AsmJitErrorHandler errorHandler;

  while (mBytes.pos() < mBytes.size()) {
    auto opcode = static_cast<Opcode>(mBytes.readU1());

    switch (opcode) {
      case Opcode::NOP: notImplemented(opcode); break;
      case Opcode::ACONST_NULL: notImplemented(opcode); break;
      case Opcode::ICONST_M1: notImplemented(opcode); break;
      case Opcode::ICONST_0: this->push(mCompiler.newInt64Const(ConstPoolScope::kGlobal, 0)); break;
      case Opcode::ICONST_1: this->push(mCompiler.newInt64Const(ConstPoolScope::kGlobal, 1)); break;
      case Opcode::ICONST_2: this->push(mCompiler.newInt64Const(ConstPoolScope::kGlobal, 2)); break;
      case Opcode::ICONST_3: this->push(mCompiler.newInt64Const(ConstPoolScope::kGlobal, 3)); break;
      case Opcode::ICONST_4: this->push(mCompiler.newInt64Const(ConstPoolScope::kGlobal, 4)); break;
      case Opcode::ICONST_5: this->push(mCompiler.newInt64Const(ConstPoolScope::kGlobal, 5)); break;
      case Opcode::LCONST_0: notImplemented(opcode); break;
      case Opcode::LCONST_1: notImplemented(opcode); break;
      case Opcode::FCONST_0: notImplemented(opcode); break;
      case Opcode::FCONST_1: notImplemented(opcode); break;
      case Opcode::FCONST_2: notImplemented(opcode); break;
      case Opcode::DCONST_0: notImplemented(opcode); break;
      case Opcode::DCONST_1: notImplemented(opcode); break;
      case Opcode::BIPUSH: notImplemented(opcode); break;
      case Opcode::SIPUSH: notImplemented(opcode); break;
      case Opcode::LDC: notImplemented(opcode); break;
      case Opcode::LDC_W: notImplemented(opcode); break;
      case Opcode::LDC2_W: notImplemented(opcode); break;
      case Opcode::ILOAD: notImplemented(opcode); break;
      case Opcode::LLOAD: notImplemented(opcode); break;
      case Opcode::FLOAD: notImplemented(opcode); break;
      case Opcode::DLOAD: notImplemented(opcode); break;
      case Opcode::ALOAD: notImplemented(opcode); break;
      case Opcode::ILOAD_0:
      case Opcode::ILOAD_1:
      case Opcode::ILOAD_2:
      case Opcode::ILOAD_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::ILOAD_0);
        this->push(qword_ptr(mLocals, slotNumber * sizeof(uint64_t)));
        break;
      }
      case Opcode::LLOAD_0: notImplemented(opcode); break;
      case Opcode::LLOAD_1: notImplemented(opcode); break;
      case Opcode::LLOAD_2: notImplemented(opcode); break;
      case Opcode::LLOAD_3: notImplemented(opcode); break;
      case Opcode::FLOAD_0: notImplemented(opcode); break;
      case Opcode::FLOAD_1: notImplemented(opcode); break;
      case Opcode::FLOAD_2: notImplemented(opcode); break;
      case Opcode::FLOAD_3: notImplemented(opcode); break;
      case Opcode::DLOAD_0:
      case Opcode::DLOAD_1:
      case Opcode::DLOAD_2:
      case Opcode::DLOAD_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::DLOAD_0);
        this->pushCategoryTwo(x86::qword_ptr(mLocals, slotNumber * sizeof(uint64_t)));
        break;
      }
      case Opcode::ALOAD_0: notImplemented(opcode); break;
      case Opcode::ALOAD_1: notImplemented(opcode); break;
      case Opcode::ALOAD_2: notImplemented(opcode); break;
      case Opcode::ALOAD_3: notImplemented(opcode); break;
      case Opcode::IALOAD: notImplemented(opcode); break;
      case Opcode::LALOAD: notImplemented(opcode); break;
      case Opcode::FALOAD: notImplemented(opcode); break;
      case Opcode::DALOAD: notImplemented(opcode); break;
      case Opcode::AALOAD: notImplemented(opcode); break;
      case Opcode::BALOAD: notImplemented(opcode); break;
      case Opcode::CALOAD: notImplemented(opcode); break;
      case Opcode::SALOAD: notImplemented(opcode); break;
      case Opcode::ISTORE: notImplemented(opcode); break;
      case Opcode::LSTORE: notImplemented(opcode); break;
      case Opcode::FSTORE: notImplemented(opcode); break;
      case Opcode::DSTORE: notImplemented(opcode); break;
      case Opcode::ASTORE: notImplemented(opcode); break;
      case Opcode::ISTORE_0:
      case Opcode::ISTORE_1:
      case Opcode::ISTORE_2:
      case Opcode::ISTORE_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::ISTORE_0);
        mCompiler.mov(qword_ptr(mLocals, slotNumber * sizeof(uint64_t)), this->pop());
        break;
      }
      case Opcode::LSTORE_0: notImplemented(opcode); break;
      case Opcode::LSTORE_1: notImplemented(opcode); break;
      case Opcode::LSTORE_2: notImplemented(opcode); break;
      case Opcode::LSTORE_3: notImplemented(opcode); break;
      case Opcode::FSTORE_0: notImplemented(opcode); break;
      case Opcode::FSTORE_1: notImplemented(opcode); break;
      case Opcode::FSTORE_2: notImplemented(opcode); break;
      case Opcode::FSTORE_3: notImplemented(opcode); break;
      case Opcode::DSTORE_0: notImplemented(opcode); break;
      case Opcode::DSTORE_1: notImplemented(opcode); break;
      case Opcode::DSTORE_2: notImplemented(opcode); break;
      case Opcode::DSTORE_3: notImplemented(opcode); break;
      case Opcode::ASTORE_0: notImplemented(opcode); break;
      case Opcode::ASTORE_1: notImplemented(opcode); break;
      case Opcode::ASTORE_2: notImplemented(opcode); break;
      case Opcode::ASTORE_3: notImplemented(opcode); break;
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
      case Opcode::DUP: notImplemented(opcode); break;
      case Opcode::DUP_X1: notImplemented(opcode); break;
      case Opcode::DUP_X2: notImplemented(opcode); break;
      case Opcode::DUP2: notImplemented(opcode); break;
      case Opcode::DUP2_X1: notImplemented(opcode); break;
      case Opcode::DUP2_X2: notImplemented(opcode); break;
      case Opcode::SWAP: notImplemented(opcode); break;
      case Opcode::IADD:
        this->binaryIntOp([this](auto& dst, auto& src) {
          mCompiler.add(dst, src);
        });
        break;
      case Opcode::LADD: notImplemented(opcode); break;
      case Opcode::FADD: notImplemented(opcode); break;
      case Opcode::DADD: {
        auto& value2 = this->popCategoryTwo();
        auto& value1 = this->popCategoryTwo();

        auto xmm2 = mCompiler.newXmm();
        auto xmm1 = mCompiler.newXmm();

        mCompiler.movq(xmm1, value1);
        mCompiler.movq(xmm2, value2);
        mCompiler.addsd(xmm1, xmm2);

        mCompiler.movq(value1, xmm1);
        mStackPointer += 2;
        break;
      }
      case Opcode::ISUB:
        this->binaryIntOp([this](auto& dst, auto& src) {
          mCompiler.sub(dst, src);
        });
        break;
      case Opcode::LSUB: notImplemented(opcode); break;
      case Opcode::FSUB: notImplemented(opcode); break;
      case Opcode::DSUB: notImplemented(opcode); break;
      case Opcode::IMUL:
        this->binaryIntOp([this](auto& dst, auto& src) {
          mCompiler.imul(dst, src);
        });
        break;
      case Opcode::LMUL: notImplemented(opcode); break;
      case Opcode::FMUL: notImplemented(opcode); break;
      case Opcode::DMUL: notImplemented(opcode); break;
      case Opcode::IDIV: notImplemented(opcode); break;
      case Opcode::LDIV: notImplemented(opcode); break;
      case Opcode::FDIV: notImplemented(opcode); break;
      case Opcode::DDIV: notImplemented(opcode); break;
      case Opcode::IREM: notImplemented(opcode); break;
      case Opcode::LREM: notImplemented(opcode); break;
      case Opcode::FREM: notImplemented(opcode); break;
      case Opcode::DREM: notImplemented(opcode); break;
      case Opcode::INEG: {
        auto& value1 = this->pop();
        mCompiler.neg(value1);
        mStackPointer++;
        break;
      }
      case Opcode::LNEG: notImplemented(opcode); break;
      case Opcode::FNEG: notImplemented(opcode); break;
      case Opcode::DNEG: notImplemented(opcode); break;
      case Opcode::ISHL: {
        auto value2 = this->pop();
        auto value1 = this->pop();

        auto offset = mCompiler.newGpd();
        mCompiler.mov(offset, mCompiler.newUInt32Const(ConstPoolScope::kGlobal, 0x1F));
        mCompiler.and_(offset, value2.r32());

        mCompiler.sal(value1.r32(), offset);
        mStackPointer++;
        break;
      }
      case Opcode::LSHL: notImplemented(opcode); break;
      case Opcode::ISHR: {
        auto value2 = this->pop();
        auto value1 = this->pop();

        auto offset = mCompiler.newGpd();
        mCompiler.mov(offset, mCompiler.newUInt32Const(ConstPoolScope::kGlobal, 0x1F));
        mCompiler.and_(offset, value2.r32());

        mCompiler.sar(value1.r32(), offset);
        mStackPointer++;
        break;
      }
      case Opcode::LSHR: notImplemented(opcode); break;
      case Opcode::IUSHR: {
        auto value2 = this->pop();
        auto value1 = this->pop();

        auto offset = mCompiler.newGpd();
        mCompiler.mov(offset, mCompiler.newUInt32Const(ConstPoolScope::kGlobal, 0x1F));
        mCompiler.and_(offset, value2.r32());

        mCompiler.shr(value1.r32(), offset);
        mStackPointer++;
        break;
      }
      case Opcode::LUSHR: notImplemented(opcode); break;
      case Opcode::IAND:
        this->binaryIntOp([this](auto& dst, auto& src) {
          mCompiler.and_(dst, src);
        });
        break;
      case Opcode::LAND: notImplemented(opcode); break;
      case Opcode::IOR:
        this->binaryIntOp([this](auto& dst, auto& src) {
          mCompiler.or_(dst, src);
        });
        break;
      case Opcode::LOR: notImplemented(opcode); break;
      case Opcode::IXOR:
        this->binaryIntOp([this](auto& dst, auto& src) {
          mCompiler.xor_(dst, src);
        });
        break;
      case Opcode::LXOR: notImplemented(opcode); break;
      case Opcode::IINC: notImplemented(opcode); break;
      case Opcode::I2L: notImplemented(opcode); break;
      case Opcode::I2F: notImplemented(opcode); break;
      case Opcode::I2D: notImplemented(opcode); break;
      case Opcode::L2I: notImplemented(opcode); break;
      case Opcode::L2F: notImplemented(opcode); break;
      case Opcode::L2D: notImplemented(opcode); break;
      case Opcode::F2I: notImplemented(opcode); break;
      case Opcode::F2L: notImplemented(opcode); break;
      case Opcode::F2D: notImplemented(opcode); break;
      case Opcode::D2I: notImplemented(opcode); break;
      case Opcode::D2L: notImplemented(opcode); break;
      case Opcode::D2F: notImplemented(opcode); break;
      case Opcode::I2B: notImplemented(opcode); break;
      case Opcode::I2C: notImplemented(opcode); break;
      case Opcode::I2S: notImplemented(opcode); break;
      case Opcode::LCMP: notImplemented(opcode); break;
      case Opcode::FCMPL: notImplemented(opcode); break;
      case Opcode::FCMPG: notImplemented(opcode); break;
      case Opcode::DCMPL: notImplemented(opcode); break;
      case Opcode::DCMPG: notImplemented(opcode); break;
      case Opcode::IFEQ: notImplemented(opcode); break;
      case Opcode::IFNE: notImplemented(opcode); break;
      case Opcode::IFLT: notImplemented(opcode); break;
      case Opcode::IFGE: notImplemented(opcode); break;
      case Opcode::IFGT: notImplemented(opcode); break;
      case Opcode::IFLE: notImplemented(opcode); break;
      case Opcode::IF_ICMPEQ: notImplemented(opcode); break;
      case Opcode::IF_ICMPNE: notImplemented(opcode); break;
      case Opcode::IF_ICMPLT: notImplemented(opcode); break;
      case Opcode::IF_ICMPGE: notImplemented(opcode); break;
      case Opcode::IF_ICMPGT: notImplemented(opcode); break;
      case Opcode::IF_ICMPLE: notImplemented(opcode); break;
      case Opcode::IF_ACMPEQ: notImplemented(opcode); break;
      case Opcode::IF_ACMPNE: notImplemented(opcode); break;
      case Opcode::GOTO: notImplemented(opcode); break;
      case Opcode::JSR: notImplemented(opcode); break;
      case Opcode::RET: notImplemented(opcode); break;
      case Opcode::TABLESWITCH: notImplemented(opcode); break;
      case Opcode::LOOKUPSWITCH: notImplemented(opcode); break;
      case Opcode::IRETURN: {
        mCompiler.ret(this->pop());
        break;
      }
      case Opcode::LRETURN: notImplemented(opcode); break;
      case Opcode::FRETURN: notImplemented(opcode); break;
      case Opcode::DRETURN: {
        mCompiler.ret(this->popCategoryTwo());
        break;
      }
      case Opcode::ARETURN: notImplemented(opcode); break;
      case Opcode::RETURN: notImplemented(opcode); break;
      case Opcode::GETSTATIC: notImplemented(opcode); break;
      case Opcode::PUTSTATIC: notImplemented(opcode); break;
      case Opcode::GETFIELD: notImplemented(opcode); break;
      case Opcode::PUTFIELD: notImplemented(opcode); break;
      case Opcode::INVOKEVIRTUAL: notImplemented(opcode); break;
      case Opcode::INVOKESPECIAL: notImplemented(opcode); break;
      case Opcode::INVOKESTATIC: notImplemented(opcode); break;
      case Opcode::INVOKEINTERFACE: notImplemented(opcode); break;
      case Opcode::INVOKEDYNAMIC: notImplemented(opcode); break;
      case Opcode::NEW: notImplemented(opcode); break;
      case Opcode::NEWARRAY: notImplemented(opcode); break;
      case Opcode::ANEWARRAY: notImplemented(opcode); break;
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

  mCompiler.endFunc();
  mCompiler.finalize();
}

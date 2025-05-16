#include "class_file/Opcode.h"
#include "common/ByteStream.h"
#include "common/Encoding.h"
#include "vm/Frame.h"
#include "vm/Vm.h"
#include "vm/jit/JitCompiler.h"

#include <asmjit/core/jitruntime.h>
#include <asmjit/core/logger.h>
#include <asmjit/x86/x86compiler.h>

#include <cmath>

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

  void invokeVirtual();
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

  template<std::derived_from<asmjit::Operand> T>
  void store(size_t idx, const T& value)
  {
    auto& target = mLocalVariables[idx];
    mCompiler.mov(target, value);
  }

  asmjit::x86::Gp& load(size_t idx)
  {
    return mLocalVariables[idx];
  }

  void binaryOp(const std::function<void(asmjit::x86::Gp&, asmjit::x86::Gp&)>& function)
  {
    auto& value2 = this->pop();
    auto& value1 = this->pop();

    function(value1, value2);
    mStackPointer++;
  }

  void binaryOpCategoryTwo(const std::function<void(asmjit::x86::Gp&, asmjit::x86::Gp&)>& function)
  {
    auto& value2 = this->popCategoryTwo();
    auto& value1 = this->popCategoryTwo();

    function(value1, value2);
    mStackPointer += 2;
  }

  void unaryJumpIf(const std::function<void(asmjit::Label& target)>& function)
  {
    auto opcodePos = mBytes.pos() - 1;

    auto& value1 = this->pop();

    auto offset = std::bit_cast<int16_t>(mBytes.readU2());
    auto label = mLabels.at(opcodePos + offset);

    mCompiler.test(value1.r32(), value1.r32());
    function(label);
  }

  void binaryJumpIf(const std::function<void(asmjit::Label& target)>& function)
  {
    auto opcodePos = mBytes.pos() - 1;

    auto& value2 = this->pop();
    auto& value1 = this->pop();

    auto offset = std::bit_cast<int16_t>(mBytes.readU2());
    auto label = mLabels.at(opcodePos + offset);

    mCompiler.cmp(value1.r32(), value2.r32());
    function(label);
  }

  void getStatic();
  void putStatic();

  asmjit::x86::Mem locals()
  {
    return qword_ptr(mCallFrame, CallFrame::LocalVariablesOffset);
  }

  void generateInitializationCall(JClass* klass);
  void generateInvoke(JMethod* method);
  void safePoint();
  void endSafePoint();

  void ldc(uint16_t index);
  void ldc2w(uint16_t index);
  void lookupSwitch();
  void tableSwitch();

  void newObject();
  void newArray();
  void newReferenceArray();

  template<class T>
  void arrayStore();

  template<class T>
  void arrayLoad()
  {
    auto& index = this->pop();
    auto& array = this->pop();

    // TODO: Null check
    // TODO: Check bounds

    static constexpr size_t ElementSize = sizeof(T);
    static constexpr size_t IndexShift = std::bit_width(JavaArray<T>::ElementIndexScale) - 1;
    static constexpr size_t IndexOffset = JavaArray<T>::ElementStartOffset;

    if (ElementSize == 8) {
      mCompiler.mov(mStack[mStackPointer++], qword_ptr(array, index, IndexShift, IndexOffset));
    } else if (ElementSize == 4) {
      mCompiler.mov(mStack[mStackPointer++].r32(), dword_ptr(array, index, IndexShift, IndexOffset));
    } else if (ElementSize == 2) {
      // CALOAD zero-extends the stored value before pushing onto the stack, the others sign-extend
      if constexpr (std::is_same_v<T, char16_t>) {
        mCompiler.movzx(mStack[mStackPointer++].r32(), word_ptr(array, index, IndexShift, IndexOffset));
      } else {
        mCompiler.movsx(mStack[mStackPointer++].r32(), word_ptr(array, index, IndexShift, IndexOffset));
      }
    } else if (ElementSize == 1) {
      mCompiler.movsx(mStack[mStackPointer++].r32(), byte_ptr(array, index, IndexShift, IndexOffset));
    }

    if constexpr (CategoryTwoJvmType<T>) {
      mStackPointer++;
    }
  }

  /// After jumps, the stack pointer can be different between target branches, so it needs adjustment.
  void adjustStackPointer();

private:
  JMethod* mMethod;
  StackMap mStackMap;
  asmjit::CodeHolder* mCode;
  asmjit::x86::Compiler mCompiler;
  ByteStream mBytes;
  asmjit::FuncNode* mFunction = nullptr;

  std::vector<asmjit::x86::Gp> mLocalVariables;
  std::vector<asmjit::x86::Gp> mStack;
  size_t mStackPointer = 0;
  asmjit::x86::Gp mThread;
  asmjit::x86::Gp mCallFrame;
  std::unordered_map<size_t, asmjit::Label> mLabels;
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
  : mMethod(method), mStackMap(StackMap::parseStackMap(mMethod)), mCode(code), mCompiler(mCode), mBytes(method->getCode().bytes())
{
  mCompiler.setLogger(mCode->logger());
  mCompiler.setErrorHandler(mCode->errorHandler());

  mFunction = mCompiler.addFunc(asmjit::FuncSignature::build<uint64_t, JavaThread*, CallFrame*>());

  mThread = mCompiler.newIntPtr();
  mCallFrame = mCompiler.newIntPtr();
  mFunction->setArg(0, mThread);
  mFunction->setArg(1, mCallFrame);

  auto localVariableMem = mCompiler.newIntPtr();
  mCompiler.mov(localVariableMem, qword_ptr(mCallFrame, CallFrame::LocalVariablesOffset));

  for (size_t i = 0; i < mMethod->getCode().maxLocals(); i++) {
    mLocalVariables.emplace_back(mCompiler.newGpq());
    mCompiler.mov(mLocalVariables[i], qword_ptr(localVariableMem, i * sizeof(uint64_t)));
  }

  for (size_t i = 0; i < mMethod->getCode().maxStack(); i++) {
    mStack.emplace_back(mCompiler.newGpq());
  }

  while (mBytes.pos() < mBytes.size()) {
    size_t pos = mBytes.pos();
    auto opcode = static_cast<Opcode>(mBytes.readU1());

    auto labelName = std::format("L#{}_{}", pos, opcodeToString(opcode));
    mLabels[pos] = mCompiler.newNamedLabel(labelName.c_str(), labelName.length());
    mBytes.skip(bytesConsumedByOpcode(opcode));
  }

  mBytes.set(0);
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
  using namespace asmjit::x86;

  AsmJitDebugLogger logger;
  logger.addFlags(FormatFlags::kMachineCode);
  logger.addFlags(FormatFlags::kHexOffsets);
  logger.addFlags(FormatFlags::kExplainImms);
  logger.addFlags(FormatFlags::kHexImms);
  logger.addFlags(FormatFlags::kPositions);
  logger.addFlags(FormatFlags::kRegCasts);
  logger.addFlags(FormatFlags::kRegType);

  AsmJitErrorHandler errorHandler;

  // Some initialization
  while (mBytes.pos() < mBytes.size()) {
    this->adjustStackPointer();
    mCompiler.bind(mLabels.at(mBytes.pos()));

    auto opcode = static_cast<Opcode>(mBytes.readU1());

    switch (opcode) {
      case Opcode::NOP: notImplemented(opcode); break;
      case Opcode::ICONST_M1: this->push(Imm{-1}); break;
      case Opcode::ACONST_NULL: [[fallthrough]];
      case Opcode::ICONST_0: this->push(Imm{0}); break;
      case Opcode::ICONST_1: this->push(Imm{1}); break;
      case Opcode::ICONST_2: this->push(Imm{2}); break;
      case Opcode::ICONST_3: this->push(Imm{3}); break;
      case Opcode::ICONST_4: this->push(Imm{4}); break;
      case Opcode::ICONST_5: this->push(Imm{5}); break;
      case Opcode::LCONST_0: this->pushCategoryTwo(Imm{0}); break;
      case Opcode::LCONST_1: this->pushCategoryTwo(Imm{1}); break;
      case Opcode::FCONST_0: notImplemented(opcode); break;
      case Opcode::FCONST_1: notImplemented(opcode); break;
      case Opcode::FCONST_2: notImplemented(opcode); break;
      case Opcode::DCONST_0: notImplemented(opcode); break;
      case Opcode::DCONST_1: notImplemented(opcode); break;
      case Opcode::BIPUSH: {
        this->push(Imm{mBytes.readU1()});
        break;
      }
      case Opcode::SIPUSH: {
        auto constantValue = mCompiler.newInt16Const(ConstPoolScope::kLocal, std::bit_cast<int16_t>(mBytes.readU2()));
        this->push(constantValue);
        break;
      }
      case Opcode::LDC: this->ldc(mBytes.readU1()); break;
      case Opcode::LDC_W: this->ldc(mBytes.readU2()); break;
      case Opcode::LDC2_W: this->ldc2w(mBytes.readU2()); break;
      case Opcode::ALOAD: [[fallthrough]];
      case Opcode::FLOAD: [[fallthrough]];
      case Opcode::ILOAD: {
        int32_t slotNumber = mBytes.readU1();
        this->push(this->load(slotNumber));
        break;
      }
      case Opcode::LLOAD: [[fallthrough]];
      case Opcode::DLOAD: {
        int32_t slotNumber = mBytes.readU1();
        this->pushCategoryTwo(this->load(slotNumber));
        break;
      }
      case Opcode::ILOAD_0:
      case Opcode::ILOAD_1:
      case Opcode::ILOAD_2:
      case Opcode::ILOAD_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::ILOAD_0);
        this->push(this->load(slotNumber));
        break;
      }
      case Opcode::LLOAD_0:
      case Opcode::LLOAD_1:
      case Opcode::LLOAD_2:
      case Opcode::LLOAD_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::LLOAD_0);
        this->pushCategoryTwo(this->load(slotNumber));
        break;
      }
      case Opcode::FLOAD_0:
      case Opcode::FLOAD_1:
      case Opcode::FLOAD_2:
      case Opcode::FLOAD_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::FLOAD_0);
        this->push(this->load(slotNumber));
        break;
      }
      case Opcode::DLOAD_0:
      case Opcode::DLOAD_1:
      case Opcode::DLOAD_2:
      case Opcode::DLOAD_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::DLOAD_0);
        this->pushCategoryTwo(this->load(slotNumber));
        break;
      }
      case Opcode::ALOAD_0:
      case Opcode::ALOAD_1:
      case Opcode::ALOAD_2:
      case Opcode::ALOAD_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::ALOAD_0);
        this->push(this->load(slotNumber));
        break;
      }
      case Opcode::IALOAD: this->arrayLoad<int32_t>(); break;
      case Opcode::FALOAD: this->arrayLoad<float>(); break;
      case Opcode::LALOAD: this->arrayLoad<int64_t>(); break;
      case Opcode::DALOAD: this->arrayLoad<double>(); break;
      case Opcode::AALOAD: this->arrayLoad<Instance*>(); break;
      case Opcode::BALOAD: this->arrayLoad<int8_t>(); break;
      case Opcode::CALOAD: this->arrayLoad<char16_t>(); break;
      case Opcode::SALOAD: this->arrayLoad<int16_t>(); break;
      case Opcode::ISTORE: {
        int32_t slotNumber = mBytes.readU1();
        this->store(slotNumber, this->pop());
        break;
      }
      case Opcode::LSTORE: notImplemented(opcode); break;
      case Opcode::FSTORE: notImplemented(opcode); break;
      case Opcode::DSTORE: notImplemented(opcode); break;
      case Opcode::ASTORE: notImplemented(opcode); break;
      case Opcode::ISTORE_0:
      case Opcode::ISTORE_1:
      case Opcode::ISTORE_2:
      case Opcode::ISTORE_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::ISTORE_0);
        this->store(slotNumber, this->pop());
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
      case Opcode::ASTORE_0:
      case Opcode::ASTORE_1:
      case Opcode::ASTORE_2:
      case Opcode::ASTORE_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::ASTORE_0);
        this->store(slotNumber, this->pop());
        break;
      }
      case Opcode::IASTORE: this->arrayStore<int32_t>(); break;
      case Opcode::FASTORE: this->arrayStore<float>(); break;
      case Opcode::LASTORE: this->arrayStore<int64_t>(); break;
      case Opcode::DASTORE: this->arrayStore<double>(); break;
      case Opcode::AASTORE: this->arrayStore<Instance*>(); break;
      case Opcode::BASTORE: this->arrayStore<int8_t>(); break;
      case Opcode::CASTORE: this->arrayStore<char16_t>(); break;
      case Opcode::SASTORE: this->arrayStore<int16_t>(); break;
      case Opcode::POP: mStackPointer--; break;
      case Opcode::POP2: mStackPointer -= 2; break;
      case Opcode::DUP: {
        auto& topOfStack = mStack[mStackPointer - 1];
        mCompiler.mov(mStack[mStackPointer++], topOfStack);
        break;
      }
      case Opcode::DUP_X1: {
        auto& value1 = mStack[mStackPointer - 1];
        auto& value2 = mStack[mStackPointer - 2];

        mCompiler.xchg(value1, value2);
        this->push(value2);
        break;
      }
      case Opcode::DUP_X2: {
        auto& value1 = mStack[mStackPointer - 1];
        auto& value2 = mStack[mStackPointer - 2];
        auto& value3 = mStack[mStackPointer - 3];

        this->push(value1);

        mCompiler.xchg(value2, value1);
        mCompiler.xchg(value3, value2);
        break;
      }
      case Opcode::DUP2: {
        auto& value1 = mStack[mStackPointer - 1];
        auto& value2 = mStack[mStackPointer - 2];

        this->push(value2);
        this->push(value1);
        break;
      }
      case Opcode::DUP2_X1: {
        // v3 v2 v1 =>
        // v2 v1 v3 v2 v1
        auto& value1 = mStack[mStackPointer - 1];
        auto& value2 = mStack[mStackPointer - 2];
        auto& value3 = mStack[mStackPointer - 3];

        // v3 v2 v1 v2 v1
        this->push(value2);
        this->push(value1);

        // v3 v1 v2 v2 v1
        mCompiler.xchg(value2, value1);
        // v2 v1 v3 v2 v1
        mCompiler.xchg(value3, value1);
        break;
      }
      case Opcode::DUP2_X2: {
        // v4 v3 v2 v1 =>
        // v2 v1 v4 v3 v2 v1
        auto& value1 = mStack[mStackPointer - 1];
        auto& value2 = mStack[mStackPointer - 2];
        auto& value3 = mStack[mStackPointer - 3];
        auto& value4 = mStack[mStackPointer - 4];

        // v4 v3 v2 v1 v2 v1
        this->push(value2);
        this->push(value1);

        // v4 v1 v2 v3 v2 v1
        mCompiler.xchg(value3, value1);
        // v2 v1 v4 v1 v2 v1
        mCompiler.xchg(value4, value2);
        break;
      }
      case Opcode::SWAP: {
        auto& value1 = mStack[mStackPointer - 1];
        auto& value2 = mStack[mStackPointer - 2];

        mCompiler.xchg(value1, value2);
        break;
      }
      case Opcode::IADD:
        this->binaryOp([this](auto& dst, auto& src) {
          mCompiler.add(dst, src);
        });
        break;
      case Opcode::LADD:
        this->binaryOpCategoryTwo([this](auto& dst, auto& src) {
          mCompiler.add(dst, src);
        });
        break;
      case Opcode::FADD: {
        this->binaryOp([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();

          mCompiler.movd(xmm1, value1);
          mCompiler.movd(xmm2, value2);
          mCompiler.addss(xmm1, xmm2);

          mCompiler.movd(value1.r32(), xmm1);
        });
        break;
      }
      case Opcode::DADD: {
        this->binaryOpCategoryTwo([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();

          mCompiler.movq(xmm1, value1);
          mCompiler.movq(xmm2, value2);
          mCompiler.addsd(xmm1, xmm2);

          mCompiler.movq(value1, xmm1);
        });
        break;
      }
      case Opcode::ISUB:
        this->binaryOp([this](auto& dst, auto& src) {
          mCompiler.sub(dst, src);
        });
        break;
      case Opcode::LSUB:
        this->binaryOpCategoryTwo([this](auto& dst, auto& src) {
          mCompiler.sub(dst, src);
        });
        break;
      case Opcode::FSUB: {
        this->binaryOp([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();

          mCompiler.movd(xmm1, value1);
          mCompiler.movd(xmm2, value2);
          mCompiler.subss(xmm1, xmm2);

          mCompiler.movd(value1.r32(), xmm1);
        });
        break;
      }
      case Opcode::DSUB: {
        this->binaryOpCategoryTwo([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();

          mCompiler.movq(xmm1, value1);
          mCompiler.movq(xmm2, value2);
          mCompiler.subsd(xmm1, xmm2);

          mCompiler.movq(value1, xmm1);
        });
        break;
      }
      case Opcode::IMUL:
        this->binaryOp([this](auto& dst, auto& src) {
          mCompiler.imul(dst, src);
        });
        break;
      case Opcode::LMUL:
        this->binaryOpCategoryTwo([this](auto& dst, auto& src) {
          mCompiler.imul(dst, src);
        });
        break;
      case Opcode::FMUL: {
        this->binaryOp([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();

          mCompiler.movd(xmm1, value1);
          mCompiler.movd(xmm2, value2);
          mCompiler.mulss(xmm1, xmm2);

          mCompiler.movd(value1.r32(), xmm1);
        });
        break;
      }
      case Opcode::DMUL: {
        this->binaryOpCategoryTwo([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();

          mCompiler.movq(xmm1, value1);
          mCompiler.movq(xmm2, value2);
          mCompiler.mulsd(xmm1, xmm2);

          mCompiler.movq(value1, xmm1);
        });
        break;
      }
      case Opcode::IDIV: {
        auto& value2 = this->pop();
        auto& value1 = this->pop();
        auto rem = mCompiler.newGpq();

        mCompiler.cdq(rem.r32(), value1.r32());
        mCompiler.idiv(rem.r32(), value1.r32(), value2.r32());
        this->push(value1);
        break;
      }
      case Opcode::LDIV: {
        auto& value2 = this->popCategoryTwo();
        auto& value1 = this->popCategoryTwo();
        auto rem = mCompiler.newGpq();

        mCompiler.cqo(rem, value1);
        mCompiler.idiv(rem, value1, value2);
        this->pushCategoryTwo(value1);
        break;
      }
      case Opcode::FDIV: {
        this->binaryOp([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();

          mCompiler.movd(xmm1, value1);
          mCompiler.movd(xmm2, value2);
          mCompiler.divss(xmm1, xmm2);

          mCompiler.movd(value1.r32(), xmm1);
        });
        break;
      }
      case Opcode::DDIV: {
        this->binaryOpCategoryTwo([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();
          mCompiler.movq(xmm1, value1);
          mCompiler.movq(xmm2, value2);
          mCompiler.divsd(xmm1, xmm2);

          mCompiler.movq(value1, xmm1);
        });
        break;
      }
      case Opcode::IREM: {
        auto& value2 = this->pop();
        auto& value1 = this->pop();
        auto rem = mCompiler.newGpq();

        mCompiler.cdq(rem.r32(), value1.r32());
        mCompiler.idiv(rem.r32(), value1.r32(), value2.r32());
        this->push(rem);
        break;
      }
      case Opcode::LREM: {
        auto& value2 = this->popCategoryTwo();
        auto& value1 = this->popCategoryTwo();
        auto rem = mCompiler.newGpq();

        mCompiler.cqo(rem, value1);
        mCompiler.idiv(rem, value1, value2);
        this->pushCategoryTwo(rem);
        break;
      }
      case Opcode::FREM: {
        this->binaryOp([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();
          mCompiler.movd(xmm2, value2.r32());
          mCompiler.movd(xmm1, value1.r32());

          float (*ptr)(float, float) = std::fmodf;

          InvokeNode* invokeNode;
          mCompiler.invoke(&invokeNode, ptr, FuncSignature::build<float, float, float>());
          invokeNode->setArg(0, xmm1);
          invokeNode->setArg(1, xmm2);
          invokeNode->setRet(0, xmm1);

          mCompiler.movd(value1.r32(), xmm1);
        });
        break;
      }
      case Opcode::DREM: {
        this->binaryOpCategoryTwo([this](auto& value1, auto& value2) {
          auto xmm2 = mCompiler.newXmm();
          auto xmm1 = mCompiler.newXmm();
          mCompiler.movq(xmm2, value2);
          mCompiler.movq(xmm1, value1);

          double (*ptr)(double, double) = std::fmod;

          InvokeNode* invokeNode;
          mCompiler.invoke(&invokeNode, ptr, FuncSignature::build<double, double, double>());
          invokeNode->setArg(0, xmm1);
          invokeNode->setArg(1, xmm2);
          invokeNode->setRet(0, xmm1);

          mCompiler.movq(value1, xmm1);
        });
        break;
      }
      case Opcode::INEG: {
        auto& value1 = this->pop();
        mCompiler.neg(value1);
        mStackPointer++;
        break;
      }
      case Opcode::LNEG: {
        auto& value1 = this->popCategoryTwo();
        mCompiler.neg(value1);
        mStackPointer += 2;
        break;
      }
      case Opcode::FNEG: {
        auto& value1 = this->pop();
        auto xmm1 = mCompiler.newXmm();
        auto xmm2 = mCompiler.newXmm();

        mCompiler.movd(xmm1, value1);
        mCompiler.movd(xmm2, mCompiler.newFloatConst(ConstPoolScope::kLocal, -0.0));
        mCompiler.xorps(xmm1, xmm2);
        mCompiler.movd(value1, xmm1);
        mStackPointer++;
        break;
      }
      case Opcode::DNEG: {
        auto& value1 = this->popCategoryTwo();
        auto xmm1 = mCompiler.newXmm();
        auto xmm2 = mCompiler.newXmm();

        mCompiler.movq(xmm1, value1);
        mCompiler.movq(xmm2, mCompiler.newDoubleConst(ConstPoolScope::kLocal, -0.0));
        mCompiler.xorpd(xmm1, xmm2);
        mCompiler.movq(value1, xmm1);
        mStackPointer += 2;
        break;
      }
      case Opcode::ISHL:
        this->binaryOp([this](auto& value1, auto& value2) {
          auto offset = mCompiler.newGpd();
          mCompiler.mov(offset, 0x1F);
          mCompiler.and_(offset, value2.r32());

          mCompiler.sal(value1.r32(), offset);
        });
        break;
      case Opcode::LSHL: {
        auto& value2 = this->pop();
        auto& value1 = this->popCategoryTwo();
        auto offset = mCompiler.newGpd();
        mCompiler.mov(offset, 0x3F);
        mCompiler.and_(offset, value2.r32());

        mCompiler.sal(value1, offset);
        mStackPointer += 2;
        break;
      }
      case Opcode::ISHR: {
        auto value2 = this->pop();
        auto value1 = this->pop();

        auto offset = mCompiler.newGpd();
        mCompiler.mov(offset, 0x1F);
        mCompiler.and_(offset, value2.r32());

        mCompiler.sar(value1.r32(), offset);
        mStackPointer++;
        break;
      }
      case Opcode::LSHR: {
        auto& value2 = this->pop();
        auto& value1 = this->popCategoryTwo();
        auto offset = mCompiler.newGpd();
        mCompiler.mov(offset, 0x3F);
        mCompiler.and_(offset, value2.r32());

        mCompiler.sar(value1, offset);
        mStackPointer += 2;
        break;
      }
      case Opcode::IUSHR: {
        auto value2 = this->pop();
        auto value1 = this->pop();

        auto offset = mCompiler.newGpd();
        mCompiler.mov(offset, 0x1F);
        mCompiler.and_(offset, value2.r32());

        mCompiler.shr(value1.r32(), offset);
        mStackPointer++;
        break;
      }
      case Opcode::LUSHR: {
        auto& value2 = this->pop();
        auto& value1 = this->popCategoryTwo();
        auto offset = mCompiler.newGpd();
        mCompiler.mov(offset, 0x3F);
        mCompiler.and_(offset, value2.r32());

        mCompiler.shr(value1, offset);
        mStackPointer += 2;
        break;
      }
      case Opcode::IAND:
        this->binaryOp([this](auto& dst, auto& src) {
          mCompiler.and_(dst, src);
        });
        break;
      case Opcode::LAND:
        this->binaryOpCategoryTwo([this](auto& dst, auto& src) {
          mCompiler.and_(dst, src);
        });
        break;
      case Opcode::IOR:
        this->binaryOp([this](auto& dst, auto& src) {
          mCompiler.or_(dst, src);
        });
        break;
      case Opcode::LOR:
        this->binaryOpCategoryTwo([this](auto& dst, auto& src) {
          mCompiler.or_(dst, src);
        });
        break;
      case Opcode::IXOR:
        this->binaryOp([this](auto& dst, auto& src) {
          mCompiler.xor_(dst, src);
        });
        break;
      case Opcode::LXOR:
        this->binaryOpCategoryTwo([this](auto& dst, auto& src) {
          mCompiler.xor_(dst, src);
        });
        break;
      case Opcode::IINC: {
        types::u1 index = mBytes.readU1();
        auto constValue = static_cast<int32_t>(std::bit_cast<int8_t>(mBytes.readU1()));

        mCompiler.add(this->load(index).r32(), constValue);
        break;
      }
      case Opcode::I2L: {
        auto& value = mStack[mStackPointer - 1];
        mCompiler.movsxd(value, value.r32());
        mStackPointer++;
        break;
      }
      case Opcode::I2F: {
        auto& value = mStack[mStackPointer - 1];
        auto xmm0 = mCompiler.newXmm();
        mCompiler.cvtsi2ss(xmm0, value.r32());
        mCompiler.movd(value.r32(), xmm0);
        break;
      }
      case Opcode::I2D: {
        auto& value = mStack[mStackPointer - 1];
        auto xmm0 = mCompiler.newXmm();
        mCompiler.cvtsi2sd(xmm0, value.r32());
        mCompiler.movq(value, xmm0);
        mStackPointer++;
        break;
      }
      case Opcode::L2I: {
        // No need to implement the cast itself, as it merely just discards the top 32-bits.
        auto& value = this->popCategoryTwo();
        this->push(value);
        break;
      }
      case Opcode::L2F: {
        auto& value = mStack[mStackPointer - 2];
        auto xmm0 = mCompiler.newXmm();
        mCompiler.cvtsi2ss(xmm0, value);
        mCompiler.movd(value.r32(), xmm0);
        mStackPointer -= 1;
        break;
      }
      case Opcode::L2D: {
        auto& value = mStack[mStackPointer - 2];
        auto xmm0 = mCompiler.newXmm();
        mCompiler.cvtsi2sd(xmm0, value);
        mCompiler.movq(value, xmm0);
        break;
      }
      case Opcode::F2I: {
        auto& value = mStack[mStackPointer - 1];

        auto pattern = mCompiler.newGpd();
        auto xmm0 = mCompiler.newXmm();
        auto xmm1 = mCompiler.newXmm();

        auto nanLabel = mCompiler.newLabel();
        auto tooBigLabel = mCompiler.newLabel();
        auto tooSmallLabel = mCompiler.newLabel();
        auto& nextInstLabel = mLabels[mBytes.pos()];

        mCompiler.movd(xmm0, value.r32());
        // Check if operand is NaN: if it is, the result is zero
        mCompiler.ucomiss(xmm0, xmm0);
        mCompiler.jp(nanLabel);
        // Round the operand using truncate
        mCompiler.roundss(xmm0, xmm0, 3);
        // If the truncated value is larger than int max
        mCompiler.mov(pattern.r32(), 0x4F000000);
        mCompiler.movd(xmm1, pattern.r32());
        mCompiler.ucomiss(xmm0, xmm1);
        mCompiler.jae(tooBigLabel);
        // If the truncated value is larger than int min
        mCompiler.mov(pattern.r32(), 0xCF000000);
        mCompiler.movd(xmm1, pattern.r32());
        mCompiler.ucomiss(xmm0, xmm1);
        mCompiler.jbe(tooSmallLabel);

        // Convert
        mCompiler.cvttss2si(value.r32(), xmm0);
        mCompiler.jmp(nextInstLabel);

        // Handle int max
        mCompiler.bind(tooBigLabel);
        mCompiler.mov(value.r32(), 0x7FFFFFFF);
        mCompiler.jmp(nextInstLabel);

        // Handle small value
        mCompiler.bind(tooSmallLabel);
        mCompiler.mov(value.r32(), 0x80000000);
        mCompiler.jmp(nextInstLabel);

        // Zero
        mCompiler.bind(nanLabel);
        mCompiler.xor_(value.r32(), value.r32());
        break;
      }
      case Opcode::F2L: {
        auto& value = mStack[mStackPointer - 1];

        auto pattern = mCompiler.newGpd();
        auto xmm0 = mCompiler.newXmm();
        auto xmm1 = mCompiler.newXmm();

        auto nanLabel = mCompiler.newLabel();
        auto tooBigLabel = mCompiler.newLabel();
        auto tooSmallLabel = mCompiler.newLabel();
        auto& nextInstLabel = mLabels[mBytes.pos()];

        mCompiler.movd(xmm0, value);
        // Check if operand is NaN: if it is, the result is zero
        mCompiler.ucomiss(xmm0, xmm0);
        mCompiler.jp(nanLabel);
        // Round the operand using truncate
        mCompiler.roundss(xmm0, xmm0, 3);
        // If the truncated value is larger than long max
        mCompiler.mov(pattern, 0x5F000000);
        mCompiler.movd(xmm1, pattern);
        mCompiler.ucomiss(xmm0, xmm1);
        mCompiler.jae(tooBigLabel);
        // If the truncated value is larger than long min
        mCompiler.mov(pattern, 0xDF000000);
        mCompiler.movd(xmm1, pattern);
        mCompiler.ucomiss(xmm0, xmm1);
        mCompiler.jbe(tooSmallLabel);

        // Convert
        mCompiler.cvttss2si(value, xmm0);
        mCompiler.jmp(nextInstLabel);

        // Handle too big value
        mCompiler.bind(tooBigLabel);
        mCompiler.mov(value, 0x7FFFFFFFFFFFFFFF);
        mCompiler.jmp(nextInstLabel);

        // Handle too small value
        mCompiler.bind(tooSmallLabel);
        mCompiler.mov(value, 0x8000000000000000);
        mCompiler.jmp(nextInstLabel);

        // Zero
        mCompiler.bind(nanLabel);
        mCompiler.xor_(value, value);

        mStackPointer++;
        break;
      }
      case Opcode::F2D: {
        auto& value = mStack[mStackPointer - 1];
        auto xmm0 = mCompiler.newXmm();

        mCompiler.movq(xmm0, value);
        mCompiler.cvtss2sd(xmm0, xmm0);
        mCompiler.movq(value, xmm0);

        mStackPointer++;
        break;
      }
      case Opcode::D2I: {
        auto& value = mStack[mStackPointer - 2];

        auto pattern = mCompiler.newGpq();
        auto xmm0 = mCompiler.newXmm();
        auto xmm1 = mCompiler.newXmm();

        auto nanLabel = mCompiler.newLabel();
        auto tooBigLabel = mCompiler.newLabel();
        auto tooSmallLabel = mCompiler.newLabel();
        auto& nextInstLabel = mLabels[mBytes.pos()];

        mCompiler.movq(xmm0, value);
        // Check if operand is NaN: if it is, the result is zero
        mCompiler.ucomisd(xmm0, xmm0);
        mCompiler.jp(nanLabel);
        // Round the operand using truncate
        mCompiler.roundsd(xmm0, xmm0, 3);
        // If the truncated value is larger than int max
        mCompiler.movabs(pattern, 0x4080000000000000);
        mCompiler.movq(xmm1, pattern);
        mCompiler.ucomisd(xmm0, xmm1);
        mCompiler.jae(tooBigLabel);
        // If the truncated value is larger than int min
        mCompiler.movabs(pattern, 0xC080000000000000);
        mCompiler.movq(xmm1, pattern);
        mCompiler.ucomisd(xmm0, xmm1);
        mCompiler.jbe(tooSmallLabel);

        // Convert
        mCompiler.cvttsd2si(value, xmm0);
        mCompiler.jmp(nextInstLabel);

        // Handle int max
        mCompiler.bind(tooBigLabel);
        mCompiler.mov(value.r32(), 0x7FFFFFFF);
        mCompiler.jmp(nextInstLabel);

        // Handle small value
        mCompiler.bind(tooSmallLabel);
        mCompiler.mov(value.r32(), 0x80000000);
        mCompiler.jmp(nextInstLabel);

        // Zero
        mCompiler.bind(nanLabel);
        mCompiler.xor_(value.r32(), value.r32());

        mStackPointer--;
        break;
      }
      case Opcode::D2L: {
        auto& value = mStack[mStackPointer - 2];

        auto pattern = mCompiler.newGpq();
        auto xmm0 = mCompiler.newXmm();
        auto xmm1 = mCompiler.newXmm();

        auto nanLabel = mCompiler.newLabel();
        auto tooBigLabel = mCompiler.newLabel();
        auto tooSmallLabel = mCompiler.newLabel();
        auto& nextInstLabel = mLabels[mBytes.pos()];

        mCompiler.movq(xmm0, value);
        // Check if operand is NaN: if it is, the result is zero
        mCompiler.ucomisd(xmm0, xmm0);
        mCompiler.jp(nanLabel);
        // Round the operand using truncate
        mCompiler.roundsd(xmm0, xmm0, 3);
        // If the truncated value is larger than long max
        mCompiler.movabs(pattern, 0x43E0000000000000);
        mCompiler.movq(xmm1, pattern);
        mCompiler.ucomisd(xmm0, xmm1);
        mCompiler.jae(tooBigLabel);
        // If the truncated value is larger than long min
        mCompiler.movabs(pattern, 0xC3E0000000000000);
        mCompiler.movq(xmm1, pattern);
        mCompiler.ucomisd(xmm0, xmm1);
        mCompiler.jbe(tooSmallLabel);

        // Convert
        mCompiler.cvttsd2si(value, xmm0);
        mCompiler.jmp(nextInstLabel);

        // Handle too big value
        mCompiler.bind(tooBigLabel);
        mCompiler.mov(value, 0x7FFFFFFFFFFFFFFF);
        mCompiler.jmp(nextInstLabel);

        // Handle too small value
        mCompiler.bind(tooSmallLabel);
        mCompiler.mov(value, 0x8000000000000000);
        mCompiler.jmp(nextInstLabel);

        // Zero
        mCompiler.bind(nanLabel);
        mCompiler.xor_(value, value);
        break;
      }
      case Opcode::D2F: {
        auto& value = mStack[mStackPointer - 2];
        auto xmm0 = mCompiler.newXmm();

        mCompiler.movq(xmm0, value);
        mCompiler.cvtsd2ss(xmm0, xmm0);
        mCompiler.movq(value, xmm0);

        mStackPointer--;
        break;
      }
      case Opcode::I2B: {
        auto& value = mStack[mStackPointer - 1];
        mCompiler.movsx(value.r32(), value.r8());
        break;
      }
      case Opcode::I2C: {
        auto& value = mStack[mStackPointer - 1];
        mCompiler.movzx(value.r32(), value.r16());
        break;
      }
      case Opcode::I2S: {
        auto& value = mStack[mStackPointer - 1];
        mCompiler.movsx(value.r32(), value.r16());
        break;
      }
      case Opcode::LCMP: {
        auto& value2 = this->popCategoryTwo();
        auto& value1 = this->popCategoryTwo();
        auto flag = mCompiler.newGpd();

        auto greaterLabel = mCompiler.newLabel();
        auto nextInst = mLabels[mBytes.pos()];

        mCompiler.cmp(value1, value2);
        mCompiler.jg(greaterLabel);
        mCompiler.xor_(flag, flag);
        mCompiler.cmp(value1, value2);
        mCompiler.setne(flag);
        mCompiler.neg(flag);
        mCompiler.mov(value1.r32(), flag);
        mCompiler.jmp(nextInst);
        mCompiler.bind(greaterLabel);
        mCompiler.mov(value1.r32(), Imm{1});
        mStackPointer++;
        break;
      }
      case Opcode::FCMPL: {
        auto& value2 = this->pop();
        auto& value1 = this->pop();

        auto xmm2 = mCompiler.newXmm();
        auto xmm1 = mCompiler.newXmm();

        mCompiler.movd(xmm1, value1.r32());
        mCompiler.movd(xmm2, value2.r32());

        auto smallerLabel = mCompiler.newLabel();
        auto nextInst = mLabels[mBytes.pos()];

        mCompiler.xor_(value1.r32(), value1.r32());
        mCompiler.ucomiss(xmm1, xmm2);
        mCompiler.jp(smallerLabel);
        mCompiler.jb(smallerLabel);
        mCompiler.setne(value1.r32());
        mCompiler.jmp(nextInst);
        mCompiler.bind(smallerLabel);
        mCompiler.mov(value1.r32(), Imm{-1});
        mStackPointer++;
        break;
      }
      case Opcode::FCMPG: {
        auto& value2 = this->pop();
        auto& value1 = this->pop();

        auto xmm2 = mCompiler.newXmm();
        auto xmm1 = mCompiler.newXmm();

        mCompiler.movd(xmm1, value1.r32());
        mCompiler.movd(xmm2, value2.r32());

        auto greaterLabel = mCompiler.newLabel();
        auto nextInst = mLabels[mBytes.pos()];

        mCompiler.xor_(value1.r32(), value1.r32());
        mCompiler.ucomiss(xmm1, xmm2);
        mCompiler.jp(greaterLabel);
        mCompiler.ja(greaterLabel);
        mCompiler.setne(value1.r32());
        mCompiler.neg(value1.r32());
        mCompiler.jmp(nextInst);
        mCompiler.bind(greaterLabel);
        mCompiler.mov(value1.r32(), Imm{1});
        mStackPointer++;
        break;
      }
      case Opcode::DCMPL: {
        auto& value2 = this->popCategoryTwo();
        auto& value1 = this->popCategoryTwo();

        auto xmm2 = mCompiler.newXmm();
        auto xmm1 = mCompiler.newXmm();

        mCompiler.movq(xmm1, value1);
        mCompiler.movq(xmm2, value2);

        auto smallerLabel = mCompiler.newLabel();
        auto nextInst = mLabels[mBytes.pos()];

        mCompiler.xor_(value1.r32(), value1.r32());
        mCompiler.ucomisd(xmm1, xmm2);
        mCompiler.jp(smallerLabel);
        mCompiler.jb(smallerLabel);
        mCompiler.setne(value1.r32());
        mCompiler.jmp(nextInst);
        mCompiler.bind(smallerLabel);
        mCompiler.mov(value1.r32(), Imm{-1});
        mStackPointer++;
        break;
      }
      case Opcode::DCMPG: {
        auto& value2 = this->popCategoryTwo();
        auto& value1 = this->popCategoryTwo();

        auto xmm2 = mCompiler.newXmm();
        auto xmm1 = mCompiler.newXmm();

        mCompiler.movq(xmm1, value1);
        mCompiler.movq(xmm2, value2);

        auto greaterLabel = mCompiler.newLabel();
        auto nextInst = mLabels[mBytes.pos()];

        mCompiler.xor_(value1.r32(), value1.r32());
        mCompiler.ucomisd(xmm1, xmm2);
        mCompiler.jp(greaterLabel);
        mCompiler.ja(greaterLabel);
        mCompiler.setne(value1.r32());
        mCompiler.neg(value1.r32());
        mCompiler.jmp(nextInst);
        mCompiler.bind(greaterLabel);
        mCompiler.mov(value1.r32(), Imm{1});
        mStackPointer++;
        break;
      }
      case Opcode::IFEQ: [[fallthrough]];
      case Opcode::IFNULL:
        this->unaryJumpIf([this](Label& label) {
          mCompiler.je(label);
        });
        break;
      case Opcode::IFNE: [[fallthrough]];
      case Opcode::IFNONNULL:
        this->unaryJumpIf([this](Label& label) {
          mCompiler.jne(label);
        });
        break;
      case Opcode::IFLT:
        this->unaryJumpIf([this](Label& label) {
          mCompiler.jl(label);
        });
        break;
      case Opcode::IFGE:
        this->unaryJumpIf([this](Label& label) {
          mCompiler.jge(label);
        });
        break;
      case Opcode::IFGT:
        this->unaryJumpIf([this](Label& label) {
          mCompiler.jg(label);
        });
        break;
      case Opcode::IFLE:
        this->unaryJumpIf([this](Label& label) {
          mCompiler.jle(label);
        });
        break;
      case Opcode::IF_ICMPEQ: [[fallthrough]];
      case Opcode::IF_ACMPEQ:
        this->binaryJumpIf([this](Label& label) {
          mCompiler.je(label);
        });
        break;
      case Opcode::IF_ICMPNE: [[fallthrough]];
      case Opcode::IF_ACMPNE:
        this->binaryJumpIf([this](Label& label) {
          mCompiler.jne(label);
        });
        break;
      case Opcode::IF_ICMPLT:
        this->binaryJumpIf([this](Label& label) {
          mCompiler.jl(label);
        });
        break;
      case Opcode::IF_ICMPGE:
        this->binaryJumpIf([this](Label& label) {
          mCompiler.jge(label);
        });
        break;
      case Opcode::IF_ICMPGT:
        this->binaryJumpIf([this](Label& label) {
          mCompiler.jg(label);
        });
        break;
      case Opcode::IF_ICMPLE:
        this->binaryJumpIf([this](Label& label) {
          mCompiler.jle(label);
        });
        break;
      case Opcode::GOTO: {
        auto opcodePos = mBytes.pos() - 1;

        auto offset = std::bit_cast<int16_t>(mBytes.readU2());
        auto label = mLabels.at(opcodePos + offset);

        mCompiler.jmp(label);
        break;
      }
      case Opcode::JSR: notImplemented(opcode); break;
      case Opcode::RET: notImplemented(opcode); break;
      case Opcode::TABLESWITCH: this->tableSwitch(); break;
      case Opcode::LOOKUPSWITCH: this->lookupSwitch(); break;
      case Opcode::ARETURN: [[fallthrough]];
      case Opcode::FRETURN: [[fallthrough]];
      case Opcode::IRETURN: {
        mCompiler.ret(this->pop());
        break;
      }
      case Opcode::LRETURN: [[fallthrough]];
      case Opcode::DRETURN: {
        mCompiler.ret(this->popCategoryTwo());
        break;
      }
      case Opcode::RETURN: {
        break;
      }
      case Opcode::GETSTATIC: {
        this->getStatic();
        break;
      }
      case Opcode::PUTSTATIC: this->putStatic(); break;
      case Opcode::GETFIELD: notImplemented(opcode); break;
      case Opcode::PUTFIELD: notImplemented(opcode); break;
      case Opcode::INVOKEVIRTUAL: this->invokeVirtual(); break;
      case Opcode::INVOKESPECIAL: {
        // TODO: Exception check
        auto index = mBytes.readU2();
        JMethod* method = mMethod->getClass()->runtimeConstantPool().getMethodRef(index);

        this->generateInvoke(method);
        break;
      }
      case Opcode::INVOKESTATIC: {
        // TODO: Exception check
        auto index = mBytes.readU2();
        JMethod* method = mMethod->getClass()->runtimeConstantPool().getMethodRef(index);
        assert(method->isStatic());

        this->generateInitializationCall(method->getClass());
        // TODO: Initialization can fail with an exception, so only proceed if an exception did not occur
        this->generateInvoke(method);

        break;
      }
      case Opcode::INVOKEINTERFACE: notImplemented(opcode); break;
      case Opcode::INVOKEDYNAMIC: notImplemented(opcode); break;
      case Opcode::NEW: this->newObject(); break;
      case Opcode::NEWARRAY: this->newArray(); break;
      case Opcode::ANEWARRAY: this->newReferenceArray(); break;
      case Opcode::ARRAYLENGTH: {
        auto& arrayRef = this->pop();
        mCompiler.mov(mStack[mStackPointer++].r32(), dword_ptr(arrayRef, ArrayInstance::LengthFieldOffset));
        break;
      }
      case Opcode::ATHROW: notImplemented(opcode); break;
      case Opcode::CHECKCAST: notImplemented(opcode); break;
      case Opcode::INSTANCEOF: notImplemented(opcode); break;
      case Opcode::MONITORENTER: notImplemented(opcode); break;
      case Opcode::MONITOREXIT: notImplemented(opcode); break;
      case Opcode::WIDE: notImplemented(opcode); break;
      case Opcode::MULTIANEWARRAY: notImplemented(opcode); break;
      case Opcode::GOTO_W: {
        auto opcodePos = mBytes.pos() - 1;

        auto offset = std::bit_cast<int32_t>(mBytes.readU4());
        auto label = mLabels.at(opcodePos + offset);

        mCompiler.jmp(label);
        break;
      }
      case Opcode::JSR_W: notImplemented(opcode); break;
      case Opcode::BREAKPOINT: notImplemented(opcode); break;
      case Opcode::IMPDEP1: notImplemented(opcode); break;
      case Opcode::IMPDEP2: notImplemented(opcode); break;
    }
  }

  mCompiler.endFunc();
  mCompiler.finalize();
}

void JitCompilerX86Impl::safePoint()
{
  auto stackAddr = mCompiler.newGpq();
  auto localsAddr = mCompiler.newGpq();
  mCompiler.mov(stackAddr, qword_ptr(mCallFrame, CallFrame::OperandStackOffset));
  mCompiler.mov(localsAddr, qword_ptr(mCallFrame, CallFrame::LocalVariablesOffset));

  for (int32_t i = 0; i < mStackPointer; i++) {
    mCompiler.mov(qword_ptr(stackAddr, i * sizeof(uint64_t)), mStack[i]);
  }

  mCompiler.mov(qword_ptr(mCallFrame, CallFrame::StackPointerOffset), mStackPointer);
  for (size_t i = 0; i < mLocalVariables.size(); i++) {
    mCompiler.mov(qword_ptr(localsAddr, i * sizeof(uint64_t)), mLocalVariables[i]);
  }
}

void JitCompilerX86Impl::endSafePoint()
{
  // Copy back from the call frame to the registers
  auto stackAddr = mCompiler.newGpq();
  auto localsAddr = mCompiler.newGpq();
  mCompiler.mov(stackAddr, qword_ptr(mCallFrame, CallFrame::OperandStackOffset));
  mCompiler.mov(localsAddr, qword_ptr(mCallFrame, CallFrame::LocalVariablesOffset));

  for (int32_t i = 0; i < mStackPointer; i++) {
    mCompiler.mov(mStack[i], qword_ptr(stackAddr, i * sizeof(uint64_t)));
  }

  for (size_t i = 0; i < mLocalVariables.size(); i++) {
    mCompiler.mov(mLocalVariables[i], qword_ptr(localsAddr, i * sizeof(uint64_t)));
  }
}

static void initializeClass(JClass* klass, JavaThread* thread)
{
  klass->initialize(*thread);
}

void JitCompilerX86Impl::generateInitializationCall(JClass* klass)
{
  // The class might already be initialized by the time this function is JIT-ted, so only generate the call if that's not the case.
  if (!klass->isInitialized()) {
    this->safePoint();

    asmjit::InvokeNode* invokeNode;
    mCompiler.invoke(&invokeNode, initializeClass, asmjit::FuncSignature::build<void, JClass*, JavaThread*>());
    invokeNode->setArg(0, klass);
    invokeNode->setArg(1, mThread);

    this->endSafePoint();
  }
}

static void invokeMethod(JMethod* method, JavaThread* thread)
{
  auto result = thread->invoke(method);
  if (!method->isVoid()) {
    thread->currentFrame().pushGenericOperand(result->toRaw());
    if (method->descriptor().returnType().getType().isCategoryTwo()) {
      thread->currentFrame().pushGenericOperand(0);
    }
  }
}

void JitCompilerX86Impl::generateInvoke(JMethod* method)
{
  this->safePoint();
  asmjit::InvokeNode* invokeNode;
  mCompiler.invoke(&invokeNode, invokeMethod, asmjit::FuncSignature::build<void, JMethod*, JavaThread*>());
  invokeNode->setArg(0, method);
  invokeNode->setArg(1, mThread);
  mStackPointer -= method->descriptor().numParameterSlots();
  if (!method->isStatic()) {
    mStackPointer -= 1;
  }

  if (!method->isVoid()) {
    mStackPointer += 1;
    if (method->descriptor().returnType().getType().isCategoryTwo()) {
      mStackPointer += 1;
    }
  }
  this->endSafePoint();
}

void JitCompilerX86Impl::getStatic()
{
  auto index = mBytes.readU2();
  const JField* field = mMethod->getClass()->runtimeConstantPool().getFieldRef(index);
  JClass* klass = field->getClass();

  // The class must be initialized not during translation, but during first execution.
  this->generateInitializationCall(klass);

  auto& target = mStack[mStackPointer++];
  if (field->fieldType().isCategoryTwo()) {
    mStackPointer++;
  }

  auto fieldPtr = mCompiler.newIntPtr();
  mCompiler.mov(fieldPtr, klass->staticFieldPtr(field->offset()));
  mCompiler.mov(target, qword_ptr(fieldPtr));
}

void JitCompilerX86Impl::putStatic()
{
  auto index = mBytes.readU2();
  const JField* field = mMethod->getClass()->runtimeConstantPool().getFieldRef(index);
  JClass* klass = field->getClass();

  // The class must be initialized not during translation, but during first execution.
  this->generateInitializationCall(klass);

  asmjit::x86::Gp* value;
  if (field->fieldType().isCategoryTwo()) {
    value = &this->popCategoryTwo();
  } else {
    value = &this->pop();
  }

  auto fieldPtr = mCompiler.newIntPtr();
  mCompiler.mov(fieldPtr, klass->staticFieldPtr(field->offset()));
  mCompiler.mov(qword_ptr(fieldPtr), *value);
}

void JitCompilerX86Impl::ldc(uint16_t index)
{
  auto& runtimeConstantPool = mMethod->getClass()->runtimeConstantPool();
  auto& [tag, data] = mMethod->getClass()->constantPool().getEntry(index);

  if (tag == ConstantPool::Tag::CONSTANT_Integer) {
    auto v = mCompiler.newInt32Const(asmjit::ConstPoolScope::kGlobal, data.singleInteger);
    this->push(v);
  } else if (tag == ConstantPool::Tag::CONSTANT_Float) {
    auto v = mCompiler.newFloatConst(asmjit::ConstPoolScope::kGlobal, data.singleFloat);
    this->push(v);
  } else if (tag == ConstantPool::Tag::CONSTANT_String) {
    this->push(asmjit::Imm{runtimeConstantPool.getString(index).get()});
  } else if (tag == ConstantPool::Tag::CONSTANT_Class) {
    auto klass = runtimeConstantPool.getClass(index);
    // TODO: Check if class is loaded
    this->push(asmjit::Imm{(*klass)->classInstance().get()});
  } else {
    GEEVM_UNREACHBLE("Unknown LDC/LDC_W type!");
  }
}

void JitCompilerX86Impl::ldc2w(uint16_t index)
{
  auto& [tag, data] = mMethod->getClass()->constantPool().getEntry(index);

  this->pushCategoryTwo(asmjit::Imm{data.doubleFloat});
}

void JitCompilerX86Impl::lookupSwitch()
{
  auto opcodePos = mBytes.pos() - 1;
  while (mBytes.pos() % 4 != 0) {
    mBytes.skip(1);
  }

  int32_t defaultOffset = std::bit_cast<int32_t>(mBytes.readU4());
  int32_t numPairs = std::bit_cast<int32_t>(mBytes.readU4());

  std::vector<std::pair<int32_t, int32_t>> pairs;
  for (int32_t i = 0; i < numPairs; i++) {
    auto matchValue = mBytes.readU4();
    auto offset = std::bit_cast<int32_t>(mBytes.readU4());
    pairs.emplace_back(matchValue, offset);
  }

  auto key = this->pop();
  for (auto& [matchValue, offset] : pairs) {
    mCompiler.cmp(key, asmjit::Imm{matchValue});
    mCompiler.je(mLabels.at(opcodePos + offset));
  }

  mCompiler.jmp(mLabels.at(opcodePos + defaultOffset));
}

void JitCompilerX86Impl::tableSwitch()
{
  auto opcodePos = mBytes.pos() - 1;
  while (mBytes.pos() % 4 != 0) {
    mBytes.skip(1);
  }

  auto defaultOffset = std::bit_cast<int32_t>(mBytes.readU4());
  auto low = std::bit_cast<int32_t>(mBytes.readU4());
  auto high = std::bit_cast<int32_t>(mBytes.readU4());
  assert(low <= high);

  int32_t count = high - low + 1;

  std::vector<int32_t> table;
  table.reserve(count);

  for (int32_t i = 0; i < count; i++) {
    int32_t offset = std::bit_cast<int32_t>(mBytes.readU4());
    table.push_back(offset);
  }

  auto index = this->pop();
  // TODO: This should be a proper table switch with indirect jumps

  mCompiler.cmp(index.r32(), asmjit::Imm{low});
  mCompiler.jl(mLabels.at(opcodePos + defaultOffset));

  auto valueToMatch = mCompiler.newGpd();
  mCompiler.mov(valueToMatch, index.r32());
  mCompiler.sub(valueToMatch, low);

  for (size_t i = 0; i < table.size(); i++) {
    mCompiler.cmp(valueToMatch, i);
    mCompiler.je(mLabels.at(opcodePos + table[i]));
  }
  mCompiler.jmp(mLabels.at(opcodePos + defaultOffset));
}

static void createNewObject(JavaThread* thread, uint16_t index)
{
  auto className = thread->currentFrame().currentClass()->constantPool().getClassName(index);

  auto klass = thread->resolveClass(types::JString{className});
  if (!klass) {
    // TODO: Throw exception
    geevm_panic("Cannot resolve class");
  }

  (*klass)->initialize(*thread);

  if (auto instanceClass = (*klass)->asInstanceClass(); instanceClass != nullptr) {
    Instance* instance = thread->heap().allocate<ObjectInstance>(instanceClass);
    thread->currentFrame().pushOperand<Instance*>(instance);
  } else {
    // TODO: New with array class
    geevm_panic("new called with array class");
  }
}

void JitCompilerX86Impl::newObject()
{
  auto index = mBytes.readU2();
  asmjit::InvokeNode* invoke;

  this->safePoint();
  mCompiler.invoke(&invoke, createNewObject, asmjit::FuncSignature::build<void, JavaThread*, uint16_t>());
  invoke->setArg(0, mThread);
  invoke->setArg(1, asmjit::Imm{index});
  mStackPointer++;
  this->endSafePoint();
}

static void createNewArray(JavaThread* thread, uint8_t kind, int32_t count)
{
  auto arrayType = static_cast<PrimitiveType>(kind);
  types::JStringRef arrayClsName = mapPrimitive(arrayType, []<PrimitiveType Type>() {
    return PrimitiveTypeTraits<Type>::ArrayClassName;
  });

  auto arrayClass = thread->resolveClass(types::JString{arrayClsName});
  // TODO:
  assert(arrayClass);
  assert(count >= 0);

  ArrayInstance* newInstance = thread->heap().allocateArray((*arrayClass)->asArrayClass(), count);
  thread->currentFrame().pushOperand<Instance*>(newInstance);
}

void JitCompilerX86Impl::newArray()
{
  uint8_t arrayType = mBytes.readU1();
  auto& count = this->pop();

  asmjit::InvokeNode* invoke;

  this->safePoint();
  mCompiler.invoke(&invoke, createNewArray, asmjit::FuncSignature::build<void, JavaThread*, int8_t, int32_t>());
  invoke->setArg(0, mThread);
  invoke->setArg(1, asmjit::Imm{arrayType});
  invoke->setArg(2, count);
  mStackPointer++;
  this->endSafePoint();
}

template<class T>
void JitCompilerX86Impl::arrayStore()
{
  asmjit::x86::Gp value;
  if constexpr (CategoryTwoJvmType<T>) {
    value = this->popCategoryTwo();
  } else {
    value = this->pop();
  }
  auto& index = this->pop();
  auto& array = this->pop();

  // TODO: Null check
  // TODO: Check bounds

  static constexpr size_t ElementSize = sizeof(T);
  static constexpr size_t IndexShift = std::bit_width(JavaArray<T>::ElementIndexScale) - 1;
  static constexpr size_t IndexOffset = JavaArray<T>::ElementStartOffset;

  if (ElementSize == 8) {
    mCompiler.mov(qword_ptr(array, index, IndexShift, IndexOffset), value);
  } else if (ElementSize == 4) {
    mCompiler.mov(dword_ptr(array, index, IndexShift, IndexOffset), value.r32());
  } else if (ElementSize == 2) {
    mCompiler.mov(word_ptr(array, index, IndexShift, IndexOffset), value.r16());
  } else if (ElementSize == 1) {
    mCompiler.mov(byte_ptr(array, index, IndexShift, IndexOffset), value.r8());
  }
}

static void createNewReferenceArray(JavaThread* thread, uint16_t index, int32_t count)
{
  auto klass = thread->currentFrame().currentClass()->runtimeConstantPool().getClass(index);

  // TODO Check class
  assert(klass);

  types::JString arrayClassName;
  if ((*klass)->isArrayType()) {
    arrayClassName = u"[" + (*klass)->className();
  } else {
    arrayClassName = u"[L" + (*klass)->className() + u";";
  }

  auto arrayClass = thread->resolveClass(arrayClassName);
  // TODO Check class
  assert(arrayClass);

  // TODO Check negative count
  ArrayInstance* newInstance = thread->heap().allocateArray((*arrayClass)->asArrayClass(), count);
  thread->currentFrame().pushOperand<Instance*>(newInstance);
}

void JitCompilerX86Impl::newReferenceArray()
{
  auto index = mBytes.readU2();
  auto& count = this->pop();

  asmjit::InvokeNode* invoke;

  this->safePoint();
  mCompiler.invoke(&invoke, createNewReferenceArray, asmjit::FuncSignature::build<void, JavaThread*, uint16_t, int32_t>());
  invoke->setArg(0, mThread);
  invoke->setArg(1, index);
  invoke->setArg(2, count);
  mStackPointer++;
  this->endSafePoint();
}

static void resolveAndInvokeVirtualMethod(JMethod* baseMethod, Instance* objectRef, JavaThread* thread)
{
  JClass* target = objectRef->getClass();
  auto targetMethod = target->getVirtualMethod(baseMethod->name(), baseMethod->rawDescriptor());

  assert(targetMethod.has_value());

  auto returnValue = thread->invoke(*targetMethod);
  if (returnValue.has_value()) {
    thread->currentFrame().pushGenericOperand(returnValue->toRaw());
  }
}

void JitCompilerX86Impl::invokeVirtual()
{
  auto index = mBytes.readU2();
  const JMethod* baseMethod = mMethod->getClass()->runtimeConstantPool().getMethodRef(index);

  int numArgs = baseMethod->descriptor().numParameterSlots();
  auto objectRef = mStack[mStackPointer - 1 - numArgs];
  // TODO: Check for null

  this->safePoint();
  asmjit::InvokeNode* invokeNode;
  mCompiler.invoke(&invokeNode, resolveAndInvokeVirtualMethod, asmjit::FuncSignature::build<void, JMethod*, Instance*, JavaThread*>());
  invokeNode->setArg(0, baseMethod);
  invokeNode->setArg(1, objectRef);
  invokeNode->setArg(2, mThread);
  mStackPointer -= baseMethod->descriptor().numParameterSlots();
  mStackPointer -= 1;

  if (!baseMethod->isVoid()) {
    mStackPointer += 1;
  }

  // TODO: Return value
  this->endSafePoint();
}

void JitCompilerX86Impl::adjustStackPointer()
{
  auto& frame = mStackMap.frameAt(mBytes.pos());
  if (frame.startPos == mBytes.pos()) {
    mStackPointer = frame.operandStack.size();
  }
}

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
#include <iostream>
#include <map>

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
  explicit JitCompilerX86Impl(JMethod* method, asmjit::CodeHolder* codeHolder, size_t exceptionPointerOffset);

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
  void getField();
  void putField();

  asmjit::x86::Mem locals()
  {
    return qword_ptr(mCallFrame, CallFrame::LocalVariablesOffset);
  }

  void generateInitializationCall(JClass* klass);
  void generateInvoke(JMethod* method);
  void safePoint();
  void endSafePoint();
  void generateExceptionHandlingCode();
  void throwException();
  void detectException(size_t opcodePos);

  void ldc(uint16_t index);
  void ldc2w(uint16_t index);
  void lookupSwitch();
  void tableSwitch();

  void newObject();
  void newArray();
  void newReferenceArray();
  void newMultiArray();

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

  void invokeVirtual();
  void invokeInterface();

  void wide();
  void checkCast();
  void instanceOf();

  /// After jumps, the stack pointer can be different between target branches, so it needs adjustment.
  void adjustStackPointer();

  [[maybe_unused]] void runtimeDebug(const asmjit::x86::Gp& value);

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
  size_t mExceptionPointerOffset;
  asmjit::x86::Gp mExceptionPointer;
  std::map<size_t, asmjit::Label> mExceptionHandlers;
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

JitCompilerX86Impl::JitCompilerX86Impl(JMethod* method, asmjit::CodeHolder* code, size_t exceptionPointerOffset)
  : mMethod(method),
    mStackMap(StackMap::parseStackMap(mMethod)),
    mCode(code),
    mCompiler(mCode),
    mBytes(method->getCode().bytes()),
    mExceptionPointerOffset(exceptionPointerOffset)
{
  mCompiler.setLogger(mCode->logger());
  mCompiler.setErrorHandler(mCode->errorHandler());

  mFunction = mCompiler.addFunc(asmjit::FuncSignature::build<uint64_t, JavaThread*, CallFrame*>());

  mThread = mCompiler.newIntPtr();
  mCallFrame = mCompiler.newIntPtr();
  mExceptionPointer = mCompiler.newIntPtr();
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

  size_t exceptionPointerOffset = mVm.mainThread().currentExceptionOffset();

  JitCompilerX86Impl impl{method, &code, exceptionPointerOffset};
  impl.doCompile();

  JitFunction fnPtr = nullptr;
  asmjit::Error err = mJitRuntime.add(&fnPtr, &code);
  if (err) {
    geevm_panic("Failed to JIT compile");
  }

  return fnPtr;
}

void JitCompilerX86Impl::detectException(size_t opcodePos)
{
  // Detect exceptions: fetch the exception pointer from the current frame.
  mCompiler.mov(mExceptionPointer, qword_ptr(mThread, mExceptionPointerOffset));
  // Jump to the exception handling logic if the exception pointer is not null
  asmjit::Label exceptionHandlingLabel;
  if (mExceptionHandlers.contains(opcodePos)) {
    exceptionHandlingLabel = mExceptionHandlers.at(opcodePos);
  } else {
    exceptionHandlingLabel = mCompiler.newLabel();
    mExceptionHandlers[opcodePos] = exceptionHandlingLabel;
  }

  mCompiler.test(mExceptionPointer, mExceptionPointer);
  mCompiler.jnz(exceptionHandlingLabel);
}
void JitCompilerX86Impl::doCompile()
{
  using namespace asmjit;

  size_t opcodePos = 0;
  while (mBytes.pos() < mBytes.size()) {
    opcodePos = mBytes.pos();
    this->adjustStackPointer();

    mCompiler.bind(mLabels.at(opcodePos));
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
      case Opcode::FCONST_0: this->push(Imm{std::bit_cast<uint32_t>(0.0f)}); break;
      case Opcode::FCONST_1: this->push(Imm{std::bit_cast<uint32_t>(1.0f)}); break;
      case Opcode::FCONST_2: this->push(Imm{std::bit_cast<uint32_t>(2.0f)}); break;
      case Opcode::DCONST_0: this->pushCategoryTwo(Imm{std::bit_cast<uint64_t>(0.0)}); break;
      case Opcode::DCONST_1: this->pushCategoryTwo(Imm{std::bit_cast<uint64_t>(1.0)}); break;
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
      case Opcode::FSTORE: [[fallthrough]];
      case Opcode::ASTORE: [[fallthrough]];
      case Opcode::ISTORE: {
        int32_t slotNumber = mBytes.readU1();
        this->store(slotNumber, this->pop());
        break;
      }
      case Opcode::LSTORE: [[fallthrough]];
      case Opcode::DSTORE: {
        int32_t slotNumber = mBytes.readU1();
        this->store(slotNumber, this->popCategoryTwo());
        break;
      }
      case Opcode::ISTORE_0: [[fallthrough]];
      case Opcode::ISTORE_1: [[fallthrough]];
      case Opcode::ISTORE_2: [[fallthrough]];
      case Opcode::ISTORE_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::ISTORE_0);
        this->store(slotNumber, this->pop());
        break;
      }
      case Opcode::LSTORE_0: [[fallthrough]];
      case Opcode::LSTORE_1: [[fallthrough]];
      case Opcode::LSTORE_2: [[fallthrough]];
      case Opcode::LSTORE_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::LSTORE_0);
        this->store(slotNumber, this->popCategoryTwo());
        break;
      }
      case Opcode::FSTORE_0: [[fallthrough]];
      case Opcode::FSTORE_1: [[fallthrough]];
      case Opcode::FSTORE_2: [[fallthrough]];
      case Opcode::FSTORE_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::FSTORE_0);
        this->store(slotNumber, this->pop());
        break;
      }
      case Opcode::DSTORE_0: [[fallthrough]];
      case Opcode::DSTORE_1: [[fallthrough]];
      case Opcode::DSTORE_2: [[fallthrough]];
      case Opcode::DSTORE_3: {
        int32_t slotNumber = static_cast<int32_t>(opcode) - static_cast<int32_t>(Opcode::DSTORE_0);
        this->store(slotNumber, this->popCategoryTwo());
        break;
      }
      case Opcode::ASTORE_0: [[fallthrough]];
      case Opcode::ASTORE_1: [[fallthrough]];
      case Opcode::ASTORE_2: [[fallthrough]];
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
        mCompiler.ret();
        break;
      }
      case Opcode::GETSTATIC: {
        this->getStatic();
        break;
      }
      case Opcode::PUTSTATIC: this->putStatic(); break;
      case Opcode::GETFIELD: this->getField(); break;
      case Opcode::PUTFIELD: this->putField(); break;
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
      case Opcode::INVOKEINTERFACE: this->invokeInterface(); break;
      case Opcode::INVOKEDYNAMIC: notImplemented(opcode); break;
      case Opcode::NEW: this->newObject(); break;
      case Opcode::NEWARRAY: this->newArray(); break;
      case Opcode::ANEWARRAY: this->newReferenceArray(); break;
      case Opcode::ARRAYLENGTH: {
        auto& arrayRef = this->pop();
        mCompiler.mov(mStack[mStackPointer++].r32(), dword_ptr(arrayRef, ArrayInstance::LengthFieldOffset));
        break;
      }
      case Opcode::ATHROW: this->throwException(); break;
      case Opcode::CHECKCAST: this->checkCast(); break;
      case Opcode::INSTANCEOF: this->instanceOf(); break;
      case Opcode::MONITORENTER: {
        mStackPointer--;
        break;
      }
      case Opcode::MONITOREXIT: {
        mStackPointer--;
        break;
      }
      case Opcode::WIDE: this->wide(); break;
      case Opcode::MULTIANEWARRAY: this->newMultiArray(); break;
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

    if (canThrowException(opcode)) {
      this->detectException(opcodePos);
    }
  }

  this->generateExceptionHandlingCode();

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

void JitCompilerX86Impl::getField()
{
  JField* field = mMethod->getClass()->runtimeConstantPool().getFieldRef(mBytes.readU2());
  auto& objectRef = this->pop();

  // TODO: Check for null
  // TODO: Check if static

  size_t fieldSize = field->fieldType().sizeOf();
  if (fieldSize == 8) {
    mCompiler.mov(mStack[mStackPointer], qword_ptr(objectRef, field->offset()));
  } else if (fieldSize == 4) {
    mCompiler.mov(mStack[mStackPointer].r32(), dword_ptr(objectRef, field->offset()));
  } else if (fieldSize == 2) {
    if (field->fieldType().asPrimitive().value() == PrimitiveType::Char) {
      mCompiler.movzx(mStack[mStackPointer].r32(), word_ptr(objectRef, field->offset()));
    } else {
      mCompiler.movsx(mStack[mStackPointer].r32(), word_ptr(objectRef, field->offset()));
    }
  } else if (fieldSize == 1) {
    mCompiler.movsx(mStack[mStackPointer].r32(), byte_ptr(objectRef, field->offset()));
  } else {
    GEEVM_UNREACHBLE("Invalid field size");
  }

  mStackPointer++;
  if (field->fieldType().isCategoryTwo()) {
    mStackPointer++;
  }
}

void JitCompilerX86Impl::putField()
{
  JField* field = mMethod->getClass()->runtimeConstantPool().getFieldRef(mBytes.readU2());
  if (field->fieldType().isCategoryTwo()) {
    mStackPointer--;
  }

  auto& value = this->pop();
  auto& objectRef = this->pop();

  // TODO: Check for null
  // TODO: Check if static

  size_t fieldSize = field->fieldType().sizeOf();
  if (fieldSize == 8) {
    mCompiler.mov(qword_ptr(objectRef, field->offset()), value);
  } else if (fieldSize == 4) {
    mCompiler.mov(dword_ptr(objectRef, field->offset()), value.r32());
  } else if (fieldSize == 2) {
    mCompiler.mov(word_ptr(objectRef, field->offset()), value.r16());
  } else if (fieldSize == 1) {
    mCompiler.mov(byte_ptr(objectRef, field->offset()), value.r8());
  } else {
    GEEVM_UNREACHBLE("Invalid field size");
  }
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

static Instance* makeMultiArray(JavaThread* thread, uint16_t index, uint8_t dimensions)
{
  auto klass = thread->currentFrame().currentClass()->runtimeConstantPool().getClass(index);
  if (!klass) {
    thread->throwException(klass.error().exception(), klass.error().message());
    return nullptr;
  }

  std::vector<int32_t> dimensionCounts;
  for (uint8_t dim = 0; dim < dimensions; dim++) {
    dimensionCounts.push_back(thread->currentFrame().popOperand<int32_t>());
  }

  auto makeInnerArray = [thread](auto& self, std::vector<int32_t> dimensionCounts, ArrayClass* arrayClass) -> GcRootRef<ArrayInstance> {
    auto count = dimensionCounts.back();
    dimensionCounts.pop_back();

    GcRootRef<ArrayInstance> newArray = nullptr;
    if (!dimensionCounts.empty()) {
      auto outerArray = thread->heap().gc().pin(thread->heap().allocateArray<Instance*>(arrayClass, count)).release();
      ArrayClass* innerArrayClass = (*arrayClass->elementClass())->asArrayClass();
      for (int32_t i = 0; i < count; i++) {
        auto innerArray = self(self, dimensionCounts, innerArrayClass);
        outerArray->setArrayElement(i, innerArray.get());
        thread->heap().gc().release(innerArray);
      }
      newArray = outerArray;
    } else {
      newArray = thread->heap().gc().pin(thread->heap().allocateArray(arrayClass, count)).release();
    }

    return newArray;
  };

  GcRootRef<ArrayInstance> result = makeInnerArray(makeInnerArray, dimensionCounts, (*klass)->asArrayClass());
  ArrayInstance* array = result.get();
  thread->heap().gc().release(result);
  return array;
}

void JitCompilerX86Impl::newMultiArray()
{
  uint16_t index = mBytes.readU2();
  uint8_t dimensions = mBytes.readU1();

  // Add a safe point to sync the operand stack
  this->safePoint();

  auto result = mCompiler.newIntPtr();
  asmjit::InvokeNode* invoke;
  mCompiler.invoke(&invoke, makeMultiArray, asmjit::FuncSignature::build<Instance*, JavaThread*, uint16_t, uint8_t>());
  invoke->setArg(0, mThread);
  invoke->setArg(1, asmjit::Imm{index});
  invoke->setArg(2, asmjit::Imm{dimensions});
  invoke->setRet(0, result);
  this->endSafePoint();

  mStackPointer -= dimensions;
  this->push(result);
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

void JitCompilerX86Impl::invokeInterface()
{
  auto index = mBytes.readU2();
  const JMethod* baseMethod = mMethod->getClass()->runtimeConstantPool().getMethodRef(index);

  // Consume 'count' and '0'
  mBytes.skip(2);

  int numArgs = baseMethod->descriptor().parameters().size();
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

static void throwExceptionInThread(JavaThread* thread, Instance* exception)
{
  thread->throwException(exception);
}

void JitCompilerX86Impl::throwException()
{
  auto& exception = this->pop();
  asmjit::InvokeNode* invoke;
  mCompiler.invoke(&invoke, throwExceptionInThread, asmjit::FuncSignature::build<void, JavaThread*, Instance*>());
  invoke->setArg(0, mThread);
  invoke->setArg(1, exception);

  // After 'ATHROW' the operand stack has only one element
  mStackPointer = 0;
  this->push(exception);
}

void JitCompilerX86Impl::wide()
{
  auto opcode = static_cast<Opcode>(mBytes.readU1());
  auto index = mBytes.readU2();

  switch (opcode) {
    using enum Opcode;
    case IINC: {
      auto constant = static_cast<int16_t>(mBytes.readU2());
      auto value = this->load(index);
      mCompiler.add(value, asmjit::Imm{constant});
      this->store(index, value);
      break;
    }
    case ALOAD: [[fallthrough]];
    case FLOAD: [[fallthrough]];
    case ILOAD: this->push(this->load(index)); break;
    case DLOAD: [[fallthrough]];
    case LLOAD: this->pushCategoryTwo(this->load(index)); break;
    case ISTORE: [[fallthrough]];
    case FSTORE: [[fallthrough]];
    case ASTORE: this->store(index, this->pop()); break;
    case LSTORE: [[fallthrough]];
    case DSTORE: this->store(index, this->popCategoryTwo()); break;
    default: GEEVM_UNREACHBLE("Unknown modified opcode for WIDE");
  }
}

static bool doCheckCast(JavaThread* thread, Instance* objectRef, types::u2 index, types::u4 pos)
{
  auto klass = thread->currentFrame().currentClass()->runtimeConstantPool().getClass(index);
  if (!klass) {
    thread->throwException(klass.error().exception(), klass.error().message());
    return true;
  }

  if (objectRef == nullptr) {
    // Nothing to do
    return false;
  }

  JClass* classToCheck = objectRef->getClass();
  if (!classToCheck->isInstanceOf(*klass)) {
    // Update the call frame with the program counter as a correct program counter value is needed for the exception line numbers
    thread->currentFrame().set(pos);

    types::JString message = u"class " + classToCheck->javaClassName() + u" cannot be cast to class " + (*klass)->javaClassName();
    thread->throwException(u"java/lang/ClassCastException", message);
    return true;
  }

  return false;
}

void JitCompilerX86Impl::checkCast()
{
  types::u2 index = mBytes.readU2();
  auto& objectRef = mStack[mStackPointer - 1];

  auto result = mCompiler.newIntPtr();

  this->safePoint();
  asmjit::InvokeNode* invoke;
  mCompiler.invoke(&invoke, doCheckCast, asmjit::FuncSignature::build<bool, JavaThread*, Instance*, types::u2, types::u4>());
  invoke->setArg(0, mThread);
  invoke->setArg(1, objectRef);
  invoke->setArg(2, asmjit::Imm{index});
  invoke->setArg(3, asmjit::Imm{mBytes.pos()});
  invoke->setRet(0, result);
  this->endSafePoint();
}

static int32_t doInstanceOf(JavaThread* thread, Instance* objectRef, types::u2 index, types::u4 pos)
{
  if (objectRef == nullptr) {
    return 0;
  }

  auto klass = thread->currentFrame().currentClass()->runtimeConstantPool().getClass(index);
  if (!klass) {
    thread->currentFrame().set(pos);
    thread->throwException(klass.error().exception(), klass.error().message());
    return 0;
  }

  JClass* classToCheck = objectRef->getClass();
  if (classToCheck->isInstanceOf(*klass)) {
    return 1;
  }
  return 0;
}

void JitCompilerX86Impl::instanceOf()
{
  types::u2 index = mBytes.readU2();
  asmjit::x86::Gp result = mCompiler.newIntPtr();

  this->safePoint();
  auto& objectRef = mStack[mStackPointer - 1];
  asmjit::InvokeNode* invoke;
  mCompiler.invoke(&invoke, doInstanceOf, asmjit::FuncSignature::build<bool, JavaThread*, Instance*, types::u2, types::u4>());
  invoke->setArg(0, mThread);
  invoke->setArg(1, objectRef);
  invoke->setArg(2, asmjit::Imm{index});
  invoke->setArg(3, asmjit::Imm{mBytes.pos()});
  invoke->setRet(0, result);
  this->endSafePoint();

  mCompiler.mov(mStack[mStackPointer - 1], result);
}

void JitCompilerX86Impl::adjustStackPointer()
{
  auto& frame = mStackMap.frameAt(mBytes.pos());
  if (frame.startPos == mBytes.pos()) {
    mStackPointer = frame.operandStack.size();
  }
}

static bool checkExceptionInstanceOf(InstanceClass* currentClass, Instance* exceptionPtr, types::u2 catchType)
{
  assert(catchType != 0);
  auto exceptionClass = currentClass->runtimeConstantPool().getClass(catchType);

  assert(exceptionClass.has_value());
  return exceptionPtr->getClass()->isInstanceOf(*exceptionClass);
}

static void clearException(JavaThread* thread)
{
  thread->clearException();
}

void JitCompilerX86Impl::generateExceptionHandlingCode()
{
  // First, generate code for individual handlers.
  // These handlers only retrieve the current program counter for the specific instruction.
  asmjit::Label exceptionHandlerLabel = mCompiler.newLabel();
  asmjit::x86::Gp pc = mCompiler.newGpq();
  for (auto& [pos, label] : mExceptionHandlers) {
    mCompiler.bind(label);
    mCompiler.mov(pc, asmjit::Imm{pos});
    // Write back the program counter to the stack frame
    mCompiler.mov(asmjit::x86::qword_ptr(mCallFrame, CallFrame::ProgramCounterOffset), pc);
    mCompiler.jmp(exceptionHandlerLabel);
  }

  // Try to handle exception in the current function.
  mCompiler.bind(exceptionHandlerLabel);

  auto exceptionInstance = mCompiler.newIntPtr();
  // The value `mExceptionPointer`fetched from the current frame is a `GcRootRef` node.
  // We need to resolve the pointer to the actual exception instance.
  mCompiler.mov(exceptionInstance, qword_ptr(mExceptionPointer, RootList::NodeInstancePointerOffset));

  // The exception instance must be at the first slot of the stack
  mCompiler.mov(mStack[0], exceptionInstance);

  for (auto& entry : mMethod->getCode().exceptionTable()) {
    auto nextIterLabel = mCompiler.newLabel();
    auto targetLabel = mLabels.at(entry.handlerPc);

    // Check if the program counter is in range for this entry
    mCompiler.cmp(pc, asmjit::Imm{entry.startPc});
    mCompiler.jl(nextIterLabel);
    mCompiler.cmp(pc, asmjit::Imm{entry.endPc});
    mCompiler.jge(nextIterLabel);
    // If control reached here, the exception handler scope is active.
    // Check if it is the right class.
    if (entry.catchType != 0) {
      auto exceptionClass = mMethod->getClass()->runtimeConstantPool().getClass(entry.catchType);
      assert(exceptionClass.has_value());

      // Note that this is safe because classes are never relocated
      asmjit::InvokeNode* invoke;
      mCompiler.invoke(&invoke, checkExceptionInstanceOf, asmjit::FuncSignature::build<bool, InstanceClass*, Instance*, types::u2>());
      invoke->setArg(0, mMethod->getClass()->asInstanceClass());
      invoke->setArg(1, exceptionInstance);
      invoke->setArg(2, asmjit::Imm{entry.catchType});
      mCompiler.jz(nextIterLabel);
    } else {
      // The exception handler is valid for all classes.
    }

    // If control reached here, then the exception was caught by the exception entry, otherwise control would have jumped to `nextIterLabel`.
    asmjit::InvokeNode* clearExceptionInvoke;
    mCompiler.invoke(&clearExceptionInvoke, clearException, asmjit::FuncSignature::build<void, JavaThread*>());
    clearExceptionInvoke->setArg(0, mThread);

    mCompiler.jmp(targetLabel);
    mCompiler.bind(nextIterLabel);
  }

  // Exception not handled in a catch block, return to caller.
  // Control reaches here if the last entry in the exception table did not handle the exception.
  mCompiler.ret();
}

static void debugCall(uint64_t value)
{
  std::cout << "DEBUG: " << value << std::endl;
}

void JitCompilerX86Impl::runtimeDebug(const asmjit::x86::Gp& value)
{
  asmjit::InvokeNode* invoke;
  mCompiler.invoke(&invoke, debugCall, asmjit::FuncSignature::build<void, uint64_t>());
  invoke->setArg(0, value);
}

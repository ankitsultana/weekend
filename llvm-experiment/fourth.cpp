// clang++ -std=c++17 jit_gte_optimized.cpp `llvm-config --cxxflags` -O2 \
//   `llvm-config --ldflags --system-libs --libs core orcjit native passes` -o jit_gte_optimized
//
// If your install splits libs differently, you might also need: analysis, transformutils, ipo.
// Example fallback (bigger hammer):
//   `llvm-config --ldflags --system-libs --libs all`
//
// Run:
//   ./jit_gte_optimized

#include <iostream>
#include <memory>
#include <vector>
#include <ctime>

#include "llvm/ADT/APInt.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/PassManager.h"

#include "llvm/Passes/PassBuilder.h"              // Modern optimization pipeline
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::orc;

// ---------- IR Builder for run_gte_comparison ----------
static Function* buildRunGte(Module& M, LLVMContext& C) {
  IRBuilder<> B(C);

  Type* I32  = Type::getInt32Ty(C);
  PointerType* I32P = PointerType::getUnqual(I32);

  // void run_gte_comparison(int* values, int length, int* comparisonResult, int testValue)
  FunctionType* FT = FunctionType::get(Type::getVoidTy(C),
                                       { I32P, I32, I32P, I32 },
                                       false);
  Function* F = Function::Create(FT, Function::ExternalLinkage,
                                 "run_gte_comparison", M);

  auto AI = F->arg_begin();
  Argument* values  = AI++; values->setName("values");
  Argument* length  = AI++; length->setName("length");
  Argument* out     = AI++; out->setName("comparisonResult");
  Argument* testVal = AI++; testVal->setName("testValue");

  BasicBlock* entryBB = BasicBlock::Create(C, "entry", F);
  BasicBlock* loopBB  = BasicBlock::Create(C, "loop",  F);
  BasicBlock* exitBB  = BasicBlock::Create(C, "exit",  F);

  IRBuilder<> BE(entryBB);
  Value* zero = ConstantInt::get(I32, 0);
  BE.CreateBr(loopBB);

  IRBuilder<> BL(loopBB);
  PHINode* i = BL.CreatePHI(I32, 2, "i");
  i->addIncoming(zero, entryBB);

  Value* inBounds = BL.CreateICmpSLT(i, length, "inbounds");
  BasicBlock* bodyBB = BasicBlock::Create(C, "body", F);
  BL.CreateCondBr(inBounds, bodyBB, exitBB);

  IRBuilder<> BB(bodyBB);
  Value* valPtr = BB.CreateInBoundsGEP(I32, values, i, "val.ptr");
  Value* val    = BB.CreateLoad(I32, valPtr, "val");
  Value* ge     = BB.CreateICmpSGE(val, testVal, "ge"); // signed >=
  Value* geI32  = BB.CreateZExt(ge, I32, "ge.i32");
  Value* outPtr = BB.CreateInBoundsGEP(I32, out, i, "out.ptr");
  BB.CreateStore(geI32, outPtr);
  Value* one    = ConstantInt::get(I32, 1);
  Value* iNext  = BB.CreateAdd(i, one, "i.next");
  BB.CreateBr(loopBB);
  i->addIncoming(iNext, bodyBB);

  IRBuilder<> BX(exitBB);
  BX.CreateRetVoid();

  if (verifyFunction(*F, &errs())) {
    errs() << "Function verification failed!\n";
  }
  return F;
}

// ---------- IR-level optimization with PassBuilder ----------
#if LLVM_VERSION_MAJOR >= 16
using OptLevelT = llvm::OptimizationLevel; // modern
#else
using OptLevelT = llvm::PassBuilder::OptimizationLevel; // older PB signature
#endif

static void optimizeModule(Module& M, OptLevelT Level) {
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;

  PassBuilder PB;

  PB.registerModuleAnalyses(MAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  ModulePassManager MPM;
#if LLVM_VERSION_MAJOR >= 16
  MPM = PB.buildPerModuleDefaultPipeline(Level);        // O0/O1/O2/O3
#else
  MPM = PB.buildPerModuleDefaultPipeline(Level);        // same name pre-16
#endif

  MPM.run(M, MAM);
}

std::vector<int> generate(int n) {
  std::vector<int> result(n);
  for (int i = 0; i < n; i++) {
    result[i] = rand() % 100000;
  }
  return result;
}

void manual(int *values, int length, int *dest, int testValue) {
  for (int idx = 0; idx < length; idx++) {
    dest[idx] = values[idx] >= testValue ? 1 : 0;
  }
}

int main() {
  // 1) Native target init for JIT
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // 2) Create the JIT with Aggressive (-O3) machine-code optimization
  auto JTMB = JITTargetMachineBuilder::detectHost();
  if (!JTMB) {
    errs() << "detectHost failed: " << toString(JTMB.takeError()) << "\n";
    return 1;
  }
  // Set the codegen optimization level for lowering (instruction selection, regalloc, etc.)
  JTMB->setCodeGenOptLevel(CodeGenOptLevel::Aggressive);

  auto JITExpected = LLJITBuilder()
      .setJITTargetMachineBuilder(std::move(*JTMB))
      .create();
  if (!JITExpected) {
    errs() << "LLJITBuilder failed: " << toString(JITExpected.takeError()) << "\n";
    return 1;
  }
  std::unique_ptr<LLJIT> J = std::move(*JITExpected);

  // 3) Build module + function
  auto Ctx = std::make_unique<LLVMContext>();
  auto Mod = std::make_unique<Module>("gte_module", *Ctx);

  // Important: Give the module the JIT's data layout before optimizing
  Mod->setDataLayout(J->getDataLayout());
  // (Optional) Target triple – often not required for JIT, but harmless:
  // Mod->setTargetTriple(sys::getProcessTriple());

  buildRunGte(*Mod, *Ctx);

  // 4) Run IR optimization at -O3
#if LLVM_VERSION_MAJOR >= 16
  optimizeModule(*Mod, OptimizationLevel::O3);
#else
  optimizeModule(*Mod, PassBuilder::OptimizationLevel::O3);
#endif

  // (Optional) View IR after optimization
  // Mod->print(outs(), nullptr);

  // 5) Add to the JIT and look up the symbol
  ThreadSafeModule TSM(std::move(Mod), std::move(Ctx));
  if (auto Err = J->addIRModule(std::move(TSM))) {
    errs() << "addIRModule failed: " << toString(std::move(Err)) << "\n";
    return 1;
  }

  auto Sym = J->lookup("run_gte_comparison");
  if (!Sym) {
    errs() << "lookup failed: " << toString(Sym.takeError()) << "\n";
    return 1;
  }

  using RunGteFn = void(*)(int*, int, int*, int);
#if LLVM_VERSION_MAJOR >= 17
  RunGteFn run_gte = Sym->toPtr<RunGteFn>();
#else
  RunGteFn run_gte = reinterpret_cast<RunGteFn>(Sym->getAddress());
#endif

  // 6) Execute like a normal function
  int n, testValue;
  std::cin >> n;
  std::cin >> testValue;
  std::vector<int> values = generate(n);
  std::vector<int> results(values.size(), 0);

  clock_t st = clock();
  run_gte(values.data(), static_cast<int>(values.size()),
          results.data(), testValue);
  clock_t en = clock();
  double gen = ((en - st) / (CLOCKS_PER_SEC * 1.0));

  st = clock();
  manual(values.data(), values.size(), results.data(), testValue);
  en = clock();
  double man = ((en - st) / (CLOCKS_PER_SEC * 1.0));
  
  std::cerr << "generated: " << gen << std::endl;
  std::cerr << "manual: " << man << std::endl;
  return 0;
}

// clang++ -std=c++17 jit_gte.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core orcjit native` -O2 -o jit_gte

#include <iostream>
#include <memory>
#include <vector>

#include "llvm/ADT/APInt.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::orc;

// === Your IR builder for run_gte_comparison (same as before) ===
static Function* buildRunGte(Module& M, LLVMContext& C) {
  IRBuilder<> B(C);

  Type* I32  = Type::getInt32Ty(C);
  PointerType* I32P = PointerType::getUnqual(I32);

  FunctionType* FT = FunctionType::get(Type::getVoidTy(C),
                                       { I32P, I32, I32P, I32 },
                                       /*isVarArg=*/false);

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
  Value* ge     = BB.CreateICmpSGE(val, testVal, "ge");
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

int main() {
  // 1) Initialize native target for JIT
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // 2) Create a context and module, build the function
  auto TSCtx = std::make_unique<LLVMContext>();
  auto M = std::make_unique<Module>("gte_module", *TSCtx);
  buildRunGte(*M, *TSCtx);

  // (Optional) See the IR
  // M->print(llvm::outs(), nullptr);

  // 3) Create LLJIT
  auto JITExpected = LLJITBuilder().create();
  if (!JITExpected) {
    errs() << "LLJITBuilder failed: " << toString(JITExpected.takeError()) << "\n";
    return 1;
  }
  std::unique_ptr<LLJIT> J = std::move(*JITExpected);

  // 4) Add our module to the JIT
  ThreadSafeModule TSM(std::move(M), std::move(TSCtx));
  if (auto Err = J->addIRModule(std::move(TSM))) {
    errs() << "addIRModule failed: " << toString(std::move(Err)) << "\n";
    return 1;
  }

  // 5) Look up the JIT-compiled symbol and cast to a function pointer
  auto Sym = J->lookup("run_gte_comparison");
  if (!Sym) {
    errs() << "lookup failed: " << toString(Sym.takeError()) << "\n";
    return 1;
  }

  using RunGteFn = void(*)(int*, int, int*, int);
#if LLVM_VERSION_MAJOR >= 17
    // LLVM 17+: lookup returns ExecutorAddr
    RunGteFn run_gte = Sym->toPtr<RunGteFn>();
#else
    // Older LLVM: lookup returns JITEvaluatedSymbol
    RunGteFn run_gte = reinterpret_cast<RunGteFn>(Sym->getAddress());
#endif

  // 6) Execute it like a normal function
  std::vector<int> values   {5, 7, 9, 2, 7, 10, -1};
  std::vector<int> results(values.size(), 0);
  int testValue = 7;

  run_gte(values.data(), static_cast<int>(values.size()),
          results.data(), testValue);

  std::cout << "values:  ";
  for (auto v : values)  std::cout << v << " ";
  std::cout << "\n>= " << testValue << " -> results: ";
  for (auto r : results) std::cout << r << " ";
  std::cout << "\n";

  // 7) You can keep `run_gte` (the function pointer) and/or keep the JIT instance
  // alive to call it again later. The pointer remains valid as long as the JIT and
  // its JITDylib remain alive (i.e., don't destroy `J`).
  //
  // Example of caching in-process:
  //   static std::unordered_map<std::string, RunGteFn> cache;
  //   cache["gte_i32"] = run_gte;

  return 0;
}

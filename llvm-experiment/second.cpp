// clang++ -std=c++17 gen_gte.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o gen_gte

#include <iostream>
#include <memory>

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

using namespace llvm;

static Function* buildRunGte(Module& M, LLVMContext& C) {
  IRBuilder<> B(C);

  // Types
  Type*        I32  = Type::getInt32Ty(C);
  PointerType* I32P = PointerType::getUnqual(I32);

  // Prototype: void run_gte_comparison(int* values, int length, int* comparisonResult, int testValue)
  FunctionType* FT = FunctionType::get(Type::getVoidTy(C),
                                       { I32P, I32, I32P, I32 },
                                       /*isVarArg=*/false);

  Function* F = Function::Create(FT, Function::ExternalLinkage,
                                 "run_gte_comparison", M);

  // Name the arguments for readability
  auto AI = F->arg_begin();
  Argument* values   = AI++;  values->setName("values");
  Argument* length   = AI++;  length->setName("length");
  Argument* out      = AI++;  out->setName("comparisonResult");
  Argument* testVal  = AI++;  testVal->setName("testValue");

  // Basic blocks
  BasicBlock* entryBB = BasicBlock::Create(C, "entry", F);
  BasicBlock* loopBB  = BasicBlock::Create(C, "loop",  F);
  BasicBlock* exitBB  = BasicBlock::Create(C, "exit",  F);

  B.SetInsertPoint(entryBB);

  // i = 0
  Value* zero = ConstantInt::get(I32, 0);
  // Jump to loop with i starting at 0
  B.CreateBr(loopBB);

  // Loop body
  B.SetInsertPoint(loopBB);

  // PHI for loop index i
  PHINode* i = B.CreatePHI(I32, 2, "i");
  i->addIncoming(zero, entryBB);

  // if (i < length) ... else exit
  Value* inBounds = B.CreateICmpSLT(i, length, "inbounds");
  // We'll split control: body then compute i_next, then branch back/exit
  // But for straight-line clarity, we can just guard the body with the cmp and branch to exit if false.
  // However, we still need a distinct block to handle the exit. We'll keep it simple:
  //   if !(i < length) goto exit
  // (Using a conditional branch to either do body or exit directly.)
  BasicBlock* bodyBB = BasicBlock::Create(C, "body", F);
  B.CreateCondBr(inBounds, bodyBB, exitBB);

  // Body: load values[i], compare >= testValue, zext to i32, store to out[i]
  B.SetInsertPoint(bodyBB);

  // GEP values[i]
  Value* valPtr = B.CreateInBoundsGEP(I32, values, i, "val.ptr");
  Value* val    = B.CreateLoad(I32, valPtr, "val");

  // cmp sge
  Value* ge     = B.CreateICmpSGE(val, testVal, "ge");
  Value* geI32  = B.CreateZExt(ge, I32, "ge.i32");  // 1 or 0

  // GEP out[i] and store result
  Value* outPtr = B.CreateInBoundsGEP(I32, out, i, "out.ptr");
  B.CreateStore(geI32, outPtr);

  // i_next = i + 1
  Value* one    = ConstantInt::get(I32, 1);
  Value* iNext  = B.CreateAdd(i, one, "i.next");

  // Backedge to loop, hooking up the PHI
  B.CreateBr(loopBB);
  i->addIncoming(iNext, bodyBB);

  // Exit: return void
  B.SetInsertPoint(exitBB);
  B.CreateRetVoid();

  // Sanity check
  if (verifyFunction(*F, &errs())) {
    errs() << "Function verification failed!\n";
  }
  return F;
}

int main() {
  LLVMContext C;
  auto M = std::make_unique<Module>("gte_module", C);

  buildRunGte(*M, C);

  // Print the generated IR
  M->print(outs(), nullptr);
  return 0;
}

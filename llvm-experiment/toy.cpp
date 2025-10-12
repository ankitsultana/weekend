#include <iostream>
#include <map>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

using namespace std;
using namespace llvm;

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<IRBuilder<>> Builder;
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;

enum BinaryComparisonOp {
    GTE,
    GT,
    LTE,
    LT,
    EQ
};

Value *LogErrorV(const char *Str) {
  cerr << Str << endl;
  return nullptr;
}

namespace JitExpressions {
class Atom {
public:
    virtual Value *codegen() = 0;
};

class ComparisonExpr : public Atom {
public:
    BinaryComparisonOp op;
    Atom *LHS;
    Atom *RHS;

    ComparisonExpr(BinaryComparisonOp op, Atom *LHS, Atom *RHS) {
        this->op = op;
        this->LHS = LHS;
        this->RHS = RHS;
    }

    virtual ~ComparisonExpr() = default;
    Value *codegen();
};

class NumberExpr : public Atom {
public:
    NumberExpr(double val) {
        this->val = val;
    }

    double val;
    virtual ~NumberExpr() = default;
    Value *codegen();
};

Value *NumberExpr::codegen() {
  return ConstantFP::get(*TheContext, APFloat(val));
}

Value *ComparisonExpr::codegen() {
    cerr << "hello" << endl;
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R) {
        return nullptr;
    }
    switch (op) {
    case BinaryComparisonOp::GT:
        return Builder->CreateFCmpOGT(L, R);
    case BinaryComparisonOp::LT:
        return Builder->CreateFCmpOLT(L, R);
    case BinaryComparisonOp::EQ:
        return Builder->CreateFCmpOEQ(L, R);
    case BinaryComparisonOp::GTE:
        return Builder->CreateFCmpOGE(L, R);
    case BinaryComparisonOp::LTE:
        return Builder->CreateFCmpOLE(L, R);
    default:
        return LogErrorV("invalid binary operator");
    }
}
}

static void InitializeModule() {
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("my cool jit", *TheContext);

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

int main() {
    InitializeModule();
    using namespace JitExpressions;
    // Create a function to hold IR so Builder has an insertion point
    FunctionType *FT =
        FunctionType::get(Type::getInt1Ty(*TheContext), /*isVarArg=*/false);
    Function *F =
        Function::Create(FT, Function::ExternalLinkage, "compare_fn", *TheModule);

    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);

    // 3.0 > 4.0
    NumberExpr l(3.0), r(4.0);
    ComparisonExpr expr(BinaryComparisonOp::GT, &l, &r);

    Value *cmp = expr.codegen(); // returns i1
    if (!cmp) {
        cerr << "codegen failed\n";
        return 1;
    }

    Builder->CreateRet(cmp);

    // Optional: verify
    if (verifyFunction(*F, &errs())) {
        cerr << "Function verification failed\n";
        return 1;
    }
    if (verifyModule(*TheModule, &errs())) {
        cerr << "Module verification failed\n";
        return 1;
    }

    TheModule->print(outs(), nullptr);
    return 0;
}
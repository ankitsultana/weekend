#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>

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

tuple<double, BinaryComparisonOp, double> AwaitComparisonInput() {
    while (true) {
        cout << "ready> ";
        cout.flush();

        double lhs = 0.0;
        double rhs = 0.0;
        string opToken;

        if (!(cin >> lhs >> opToken >> rhs)) {
            throw runtime_error("Failed to read comparison input");
        }

        BinaryComparisonOp op;
        if (opToken == ">") {
            op = BinaryComparisonOp::GT;
        } else if (opToken == "<") {
            op = BinaryComparisonOp::LT;
        } else if (opToken == "==") {
            op = BinaryComparisonOp::EQ;
        } else if (opToken == ">=") {
            op = BinaryComparisonOp::GTE;
        } else if (opToken == "<=") {
            op = BinaryComparisonOp::LTE;
        } else {
            cerr << "Unknown comparison operator: " << opToken << endl;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        return make_tuple(lhs, op, rhs);
    }
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

    auto comparisonInput = AwaitComparisonInput();
    double lhsValue = get<0>(comparisonInput);
    BinaryComparisonOp op = get<1>(comparisonInput);
    double rhsValue = get<2>(comparisonInput);

    // Create a function to hold IR so Builder has an insertion point
    FunctionType *FT =
        FunctionType::get(Type::getInt1Ty(*TheContext), /*isVarArg=*/false);
    Function *F =
        Function::Create(FT, Function::ExternalLinkage, "compare_fn", *TheModule);

    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);

    NumberExpr lhsExpr(lhsValue), rhsExpr(rhsValue);
    ComparisonExpr expr(op, &lhsExpr, &rhsExpr);

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

#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

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

static bool EvaluateComparison(double lhs, double rhs, BinaryComparisonOp op) {
    switch (op) {
    case BinaryComparisonOp::GT:
        return lhs > rhs;
    case BinaryComparisonOp::LT:
        return lhs < rhs;
    case BinaryComparisonOp::EQ:
        return lhs == rhs;
    case BinaryComparisonOp::GTE:
        return lhs >= rhs;
    case BinaryComparisonOp::LTE:
        return lhs <= rhs;
    default:
        throw invalid_argument("Unsupported binary comparison operator");
    }
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

class ArrayComparisonExpr : public Atom {
public:
    using Comparator = std::function<bool(int)>;

    ArrayComparisonExpr(const int *values, size_t length, Comparator comparator)
        : comparator(std::move(comparator)) {
        if (values != nullptr && length > 0) {
            data.assign(values, values + length);
        }
    }

    virtual ~ArrayComparisonExpr() = default;
    Value *codegen();

private:
    std::vector<int> data;
    Comparator comparator;
};

Value *NumberExpr::codegen() {
  return ConstantFP::get(*TheContext, APFloat(val));
}

Value *ArrayComparisonExpr::codegen() {
    if (!comparator) {
        return LogErrorV("ArrayComparisonExpr comparator not set");
    }

    if (!TheModule) {
        return LogErrorV("Module not initialized");
    }

    LLVMContext &context = *TheContext;
    Type *boolTy = Type::getInt1Ty(context);

    if (data.empty()) {
        return ConstantPointerNull::get(boolTy->getPointerTo());
    }

    std::vector<Constant *> boolConstants;
    boolConstants.reserve(data.size());
    for (int value : data) {
        bool result = comparator(value);
        boolConstants.push_back(ConstantInt::get(boolTy, result ? 1 : 0));
    }

    ArrayType *arrayTy = ArrayType::get(boolTy, boolConstants.size());
    Constant *constArray = ConstantArray::get(arrayTy, boolConstants);

    static unsigned globalId = 0;
    string globalName = "array_cmp_results_" + to_string(globalId++);

    GlobalVariable *global = new GlobalVariable(
        *TheModule, arrayTy, /*isConstant=*/true, GlobalValue::PrivateLinkage, constArray,
        globalName);
    global->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);

    Constant *zero32 = ConstantInt::get(Type::getInt32Ty(context), 0);
    Constant *indices[] = {zero32, zero32};

    return ConstantExpr::getInBoundsGetElementPtr(arrayTy, global, indices);
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

static int RunComparisonSession() {
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

    constexpr size_t arrayLength = 5;
    unique_ptr<int[]> dynamicArray(new int[arrayLength]);
    for (size_t i = 0; i < arrayLength; ++i) {
        dynamicArray[i] = static_cast<int>(lhsValue) + static_cast<int>(i);
    }

    auto comparatorFn = [op, rhsValue](int element) {
        return EvaluateComparison(static_cast<double>(element), rhsValue, op);
    };

    ArrayComparisonExpr arrayExpr(dynamicArray.get(), arrayLength, comparatorFn);
    Value *boolArrayPtr = arrayExpr.codegen();
    if (!boolArrayPtr) {
        cerr << "array comparison codegen failed\n";
        return 1;
    }

    Value *cmp = expr.codegen(); // returns i1
    if (!cmp) {
        cerr << "codegen failed\n";
        return 1;
    }

    Value *arrayPtrSlot =
        Builder->CreateAlloca(boolArrayPtr->getType(), nullptr, "comparison_results");
    Builder->CreateStore(boolArrayPtr, arrayPtrSlot);

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

int main() {
    try {
        return RunComparisonSession();
    } catch (const std::exception &ex) {
        cerr << ex.what() << endl;
        return 1;
    }
}

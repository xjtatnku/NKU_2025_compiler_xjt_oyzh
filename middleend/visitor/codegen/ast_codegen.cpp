#include <middleend/visitor/codegen/ast_codegen.h>
#include <debug.h>
#include <functional>

namespace ME
{
    void ASTCodeGen::libFuncRegister(Module* m)
    {
        auto& decls = m->funcDecls;

        // int getint();
        decls.emplace_back(new FuncDeclInst(DataType::I32, "getint"));

        // int getch();
        decls.emplace_back(new FuncDeclInst(DataType::I32, "getch"));

        // int getarray(int a[]);
        decls.emplace_back(new FuncDeclInst(DataType::I32, "getarray", {DataType::PTR}));

        // float getfloat();
        decls.emplace_back(new FuncDeclInst(DataType::F32, "getfloat"));

        // int getfarray(float a[]);
        decls.emplace_back(new FuncDeclInst(DataType::I32, "getfarray", {DataType::F32_PTR}));

        // void putint(int a);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putint", {DataType::I32}));

        // void putch(int a);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putch", {DataType::I32}));

        // void putarray(int n, int a[]);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putarray", {DataType::I32, DataType::PTR}));

        // void putfloat(float a);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putfloat", {DataType::F32}));

        // void putfarray(int n, float a[]);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "putfarray", {DataType::I32, DataType::F32_PTR}));

        // void starttime(int lineno);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "_sysy_starttime", {DataType::I32}));

        // void stoptime(int lineno);
        decls.emplace_back(new FuncDeclInst(DataType::VOID, "_sysy_stoptime", {DataType::I32}));

        // llvm memset
        decls.emplace_back(new FuncDeclInst(
            DataType::VOID, "llvm.memset.p0.i32", {DataType::PTR, DataType::I8, DataType::I32, DataType::I1}));
    }

    void ASTCodeGen::handleGlobalVarDecl(FE::AST::VarDeclStmt* decls, Module* m)
    {
        // TODO(Lab 3-2): 生成全局变量声明 IR（支持标量与数组的初值）
        if (!decls || !decls->decl) return;
        auto* varDecl = decls->decl;
        
        for (auto* decl : *varDecl->decls) {
            if (!decl) continue;
            FE::AST::LeftValExpr* lval = dynamic_cast<FE::AST::LeftValExpr*>(decl->lval);
            if (!lval) continue;
            
            std::string name = lval->entry->getName();
            const auto& attr = glbSymbols.at(lval->entry);
            DataType finalType = convert(varDecl->type);
            
            if (attr.arrayDims.empty()) {
                // Scalar
                Operand* initOp = nullptr;
                if (decl->init) {
                    FE::AST::Initializer* simpleInit = dynamic_cast<FE::AST::Initializer*>(decl->init);
                    if (simpleInit && simpleInit->init_val && simpleInit->init_val->attr.val.isConstexpr) {
                        if (finalType == DataType::I32) {
                            initOp = getImmeI32Operand(simpleInit->init_val->attr.val.getInt());
                        } else if (finalType == DataType::F32) {
                            initOp = getImmeF32Operand(simpleInit->init_val->attr.val.getFloat());
                        }
                    }
                }
                if (!initOp) {
                    // Zero init
                    if (finalType == DataType::I32) initOp = getImmeI32Operand(0);
                    else if (finalType == DataType::F32) initOp = getImmeF32Operand(0.0f);
                }
                m->globalVars.push_back(new GlbVarDeclInst(finalType, name, initOp));
            } else {
                // Array
                FE::AST::VarAttr arrayAttr = attr;  // Copy to modify initList
                
                // Process initialization list if present
                if (decl->init) {
                    FE::AST::InitializerList* initList = dynamic_cast<FE::AST::InitializerList*>(decl->init);
                    if (initList) {
                        // Flatten initialization list into initList using recursive helper
                        struct FlattenHelper {
                            FE::AST::VarAttr& attr;
                            FlattenHelper(FE::AST::VarAttr& a) : attr(a) {}
                            void flatten(FE::AST::InitDecl* init) {
                                if (!init) return;
                                FE::AST::Initializer* simple = dynamic_cast<FE::AST::Initializer*>(init);
                                FE::AST::InitializerList* list = dynamic_cast<FE::AST::InitializerList*>(init);
                                if (simple && simple->init_val) {
                                    // Only add constant values to initList
                                    if (simple->init_val->attr.val.isConstexpr) {
                                        attr.initList.push_back(simple->init_val->attr.val.value);
                                    }
                                } else if (list && list->init_list) {
                                    for (auto* subInit : *(list->init_list)) {
                                        flatten(subInit);
                                    }
                                }
                            }
                        };
                        FlattenHelper helper(arrayAttr);
                        helper.flatten(initList);
                    }
                }
                
                m->globalVars.push_back(new GlbVarDeclInst(finalType, name, arrayAttr));
            }
        }
    }

    void ASTCodeGen::visit(FE::AST::Root& node, Module* m)
    {
        // 示例：注册库函数
        libFuncRegister(m);

        // TODO(Lab 3-2): 生成模块级 IR
        // 处理顶层语句：全局变量声明、函数定义等
        if (node.getStmts()) {
            for (auto* stmt : *node.getStmts()) {
                if (stmt) {
                    if (auto* varDecl = dynamic_cast<FE::AST::VarDeclStmt*>(stmt)) {
                        handleGlobalVarDecl(varDecl, m);
                    } else if (auto* funcDecl = dynamic_cast<FE::AST::FuncDeclStmt*>(stmt)) {
                        visit(*funcDecl, m);
                    } else {
                        ERROR("Invalid statement in root");
                    }
                }
            }
        }
    }

    void ASTCodeGen::dispatch(FE::AST::StmtNode* stmt, Module* m)
    {
        if (!stmt) return;
        if (auto* s = dynamic_cast<FE::AST::ExprStmt*>(stmt)) visit(*s, m);
        else if (auto* s = dynamic_cast<FE::AST::FuncDeclStmt*>(stmt)) visit(*s, m);
        else if (auto* s = dynamic_cast<FE::AST::VarDeclStmt*>(stmt)) visit(*s, m);
        else if (auto* s = dynamic_cast<FE::AST::BlockStmt*>(stmt)) visit(*s, m);
        else if (auto* s = dynamic_cast<FE::AST::ReturnStmt*>(stmt)) visit(*s, m);
        else if (auto* s = dynamic_cast<FE::AST::WhileStmt*>(stmt)) visit(*s, m);
        else if (auto* s = dynamic_cast<FE::AST::IfStmt*>(stmt)) visit(*s, m);
        else if (auto* s = dynamic_cast<FE::AST::BreakStmt*>(stmt)) visit(*s, m);
        else if (auto* s = dynamic_cast<FE::AST::ContinueStmt*>(stmt)) visit(*s, m);
        else if (auto* s = dynamic_cast<FE::AST::ForStmt*>(stmt)) visit(*s, m);
        else ERROR("Unknown statement type");
    }

    void ASTCodeGen::dispatch(FE::AST::ExprNode* expr, Module* m)
    {
        if (!expr) return;
        if (auto* e = dynamic_cast<FE::AST::LeftValExpr*>(expr)) visit(*e, m);
        else if (auto* e = dynamic_cast<FE::AST::LiteralExpr*>(expr)) visit(*e, m);
        else if (auto* e = dynamic_cast<FE::AST::UnaryExpr*>(expr)) visit(*e, m);
        else if (auto* e = dynamic_cast<FE::AST::BinaryExpr*>(expr)) visit(*e, m);
        else if (auto* e = dynamic_cast<FE::AST::CallExpr*>(expr)) visit(*e, m);
        else if (auto* e = dynamic_cast<FE::AST::CommaExpr*>(expr)) visit(*e, m);
        else ERROR("Unknown expression type");
    }

    LoadInst* ASTCodeGen::createLoadInst(DataType t, Operand* ptr, size_t resReg)
    {
        return new LoadInst(t, ptr, getRegOperand(resReg));
    }

    StoreInst* ASTCodeGen::createStoreInst(DataType t, size_t valReg, Operand* ptr)
    {
        return new StoreInst(t, getRegOperand(valReg), ptr);
    }
    StoreInst* ASTCodeGen::createStoreInst(DataType t, Operand* val, Operand* ptr)
    {
        return new StoreInst(t, val, ptr);
    }

    ArithmeticInst* ASTCodeGen::createArithmeticI32Inst(Operator op, size_t lhsReg, size_t rhsReg, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::I32, getRegOperand(lhsReg), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticI32Inst_ImmeLeft(Operator op, int lhsVal, size_t rhsReg, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::I32, getImmeI32Operand(lhsVal), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticI32Inst_ImmeAll(Operator op, int lhsVal, int rhsVal, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::I32, getImmeI32Operand(lhsVal), getImmeI32Operand(rhsVal), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticF32Inst(Operator op, size_t lhsReg, size_t rhsReg, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::F32, getRegOperand(lhsReg), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticF32Inst_ImmeLeft(
        Operator op, float lhsVal, size_t rhsReg, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::F32, getImmeF32Operand(lhsVal), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    ArithmeticInst* ASTCodeGen::createArithmeticF32Inst_ImmeAll(Operator op, float lhsVal, float rhsVal, size_t resReg)
    {
        return new ArithmeticInst(
            op, DataType::F32, getImmeF32Operand(lhsVal), getImmeF32Operand(rhsVal), getRegOperand(resReg));
    }

    IcmpInst* ASTCodeGen::createIcmpInst(ICmpOp cond, size_t lhsReg, size_t rhsReg, size_t resReg)
    {
        return new IcmpInst(DataType::I32, cond, getRegOperand(lhsReg), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    IcmpInst* ASTCodeGen::createIcmpInst_ImmeRight(ICmpOp cond, size_t lhsReg, int rhsVal, size_t resReg)
    {
        return new IcmpInst(
            DataType::I32, cond, getRegOperand(lhsReg), getImmeI32Operand(rhsVal), getRegOperand(resReg));
    }
    FcmpInst* ASTCodeGen::createFcmpInst(FCmpOp cond, size_t lhsReg, size_t rhsReg, size_t resReg)
    {
        return new FcmpInst(DataType::F32, cond, getRegOperand(lhsReg), getRegOperand(rhsReg), getRegOperand(resReg));
    }
    FcmpInst* ASTCodeGen::createFcmpInst_ImmeRight(FCmpOp cond, size_t lhsReg, float rhsVal, size_t resReg)
    {
        return new FcmpInst(
            DataType::F32, cond, getRegOperand(lhsReg), getImmeF32Operand(rhsVal), getRegOperand(resReg));
    }

    FP2SIInst* ASTCodeGen::createFP2SIInst(size_t srcReg, size_t destReg)
    {
        return new FP2SIInst(getRegOperand(srcReg), getRegOperand(destReg));
    }
    SI2FPInst* ASTCodeGen::createSI2FPInst(size_t srcReg, size_t destReg)
    {
        return new SI2FPInst(getRegOperand(srcReg), getRegOperand(destReg));
    }
    ZextInst* ASTCodeGen::createZextInst(size_t srcReg, size_t destReg, size_t srcBits, size_t destBits)
    {
        ASSERT(srcBits == 1 && destBits == 32 && "Currently only support i1 to i32 zext");
        return new ZextInst(DataType::I1, DataType::I32, getRegOperand(srcReg), getRegOperand(destReg));
    }

    GEPInst* ASTCodeGen::createGEP_I32Inst(
        DataType t, Operand* ptr, std::vector<int> dims, std::vector<Operand*> is, size_t resReg)
    {
        return new GEPInst(t, DataType::I32, ptr, getRegOperand(resReg), dims, is);
    }

    size_t ASTCodeGen::linearizeIndices(const std::vector<int>& dims, const std::vector<size_t>& idxRegs)
    {
        ASSERT(!idxRegs.empty());
        size_t offsetReg = idxRegs[0];
        for (size_t i = 1; i < idxRegs.size(); ++i) {
            int dimSize = 1;
            if (i - 1 < dims.size() && dims[i - 1] > 0) dimSize = dims[i - 1];

            size_t mulReg = getNewRegId();
            insert(createArithmeticI32Inst_ImmeLeft(Operator::MUL, dimSize, offsetReg, mulReg));

            size_t addReg = getNewRegId();
            insert(createArithmeticI32Inst(Operator::ADD, mulReg, idxRegs[i], addReg));
            offsetReg = addReg;
        }
        return offsetReg;
    }

    std::vector<Operand*> ASTCodeGen::buildZeroIndexList(size_t count)
    {
        std::vector<Operand*> idxs;
        idxs.reserve(count);
        for (size_t i = 0; i < count; ++i) idxs.push_back(getImmeI32Operand(0));
        return idxs;
    }

    CallInst* ASTCodeGen::createCallInst(DataType t, std::string funcName, CallInst::argList args, size_t resReg)
    {
        return new CallInst(t, funcName, args, getRegOperand(resReg));
    }
    CallInst* ASTCodeGen::createCallInst(DataType t, std::string funcName, CallInst::argList args)
    {
        return new CallInst(t, funcName, args);
    }
    CallInst* ASTCodeGen::createCallInst(DataType t, std::string funcName, size_t resReg)
    {
        return new CallInst(t, funcName, getRegOperand(resReg));
    }
    CallInst* ASTCodeGen::createCallInst(DataType t, std::string funcName) { return new CallInst(t, funcName); }

    RetInst* ASTCodeGen::createRetInst() { return new RetInst(DataType::VOID); }
    RetInst* ASTCodeGen::createRetInst(DataType t, size_t retReg) { return new RetInst(t, getRegOperand(retReg)); }
    RetInst* ASTCodeGen::createRetInst(int val) { return new RetInst(DataType::I32, getImmeI32Operand(val)); }
    RetInst* ASTCodeGen::createRetInst(float val) { return new RetInst(DataType::F32, getImmeF32Operand(val)); }

    BrCondInst* ASTCodeGen::createBranchInst(size_t condReg, size_t trueTar, size_t falseTar)
    {
        return new BrCondInst(getRegOperand(condReg), getLabelOperand(trueTar), getLabelOperand(falseTar));
    }
    BrUncondInst* ASTCodeGen::createBranchInst(size_t tar) { return new BrUncondInst(getLabelOperand(tar)); }

    AllocaInst* ASTCodeGen::createAllocaInst(DataType t, size_t ptrReg)
    {
        return new AllocaInst(t, getRegOperand(ptrReg));
    }
    AllocaInst* ASTCodeGen::createAllocaInst(DataType t, size_t ptrReg, std::vector<int> dims)
    {
        return new AllocaInst(t, getRegOperand(ptrReg), dims);
    }

    std::list<Instruction*> ASTCodeGen::createTypeConvertInst(DataType from, DataType to, size_t srcReg)
    {
        if (from == to) return {};
        ASSERT((from == DataType::I1) || (from == DataType::I32) || (from == DataType::F32));
        ASSERT((to == DataType::I1) || (to == DataType::I32) || (to == DataType::F32));

        std::list<Instruction*> insts;

        switch (from)
        {
            case DataType::I1:
            {
                switch (to)
                {
                    case DataType::I32:
                    {
                        size_t    destReg = getNewRegId();
                        ZextInst* zext    = createZextInst(srcReg, destReg, 1, 32);
                        insts.push_back(zext);
                        break;
                    }
                    case DataType::F32:
                    {
                        size_t    i32Reg = getNewRegId();
                        ZextInst* zext   = createZextInst(srcReg, i32Reg, 1, 32);
                        insts.push_back(zext);
                        size_t f32Reg = getNewRegId();
                        insts.push_back(createSI2FPInst(i32Reg, f32Reg));
                        break;
                    }
                    default: ERROR("Type conversion not supported");
                }
                break;
            }
            case DataType::I32:
            {
                switch (to)
                {
                    case DataType::I1:
                    {
                        size_t    destReg = getNewRegId();
                        IcmpInst* icmp    = createIcmpInst_ImmeRight(ICmpOp::NE, srcReg, 0, destReg);
                        insts.push_back(icmp);
                        break;
                    }
                    case DataType::F32:
                    {
                        size_t destReg = getNewRegId();
                        insts.push_back(createSI2FPInst(srcReg, destReg));
                        break;
                    }
                    default: ERROR("Type conversion not supported");
                }
                break;
            }
            case DataType::F32:
            {
                switch (to)
                {
                    case DataType::I1:
                    {
                        size_t    destReg = getNewRegId();
                        FcmpInst* fcmp    = createFcmpInst_ImmeRight(FCmpOp::ONE, srcReg, 0.0f, destReg);
                        insts.push_back(fcmp);
                        break;
                    }
                    case DataType::I32:
                    {
                        size_t destReg = getNewRegId();
                        insts.push_back(createFP2SIInst(srcReg, destReg));
                        break;
                    }
                    default: ERROR("Type conversion not supported");
                }
                break;
            }
            default: ERROR("Type conversion not supported");
        }

        return insts;
    }
}  // namespace ME

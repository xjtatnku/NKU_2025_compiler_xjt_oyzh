#include <middleend/visitor/codegen/ast_codegen.h>
#include <debug.h>
#include <functional>

namespace ME
{
    void ASTCodeGen::visit(FE::AST::Initializer& node, Module* m)
    {
        (void)m;
        ERROR("Initializer should not appear here, at line %d", node.line_num);
    }
    void ASTCodeGen::visit(FE::AST::InitializerList& node, Module* m)
    {
        (void)m;
        ERROR("InitializerList should not appear here, at line %d", node.line_num);
    }
    void ASTCodeGen::visit(FE::AST::VarDeclarator& node, Module* m)
    {
        (void)m;
        ERROR("VarDeclarator should not appear here, at line %d", node.line_num);
    }
    void ASTCodeGen::visit(FE::AST::ParamDeclarator& node, Module* m)
    {
        (void)m;
        ERROR("ParamDeclarator should not appear here, at line %d", node.line_num);
    }

    void ASTCodeGen::visit(FE::AST::VarDeclaration& node, Module* m)
    {
        // TODO(Lab 3-2): 生成变量声明 IR（alloca、数组零初始化、可选初始化表达式）
        if (!node.decls) return;
        for (auto* decl : *node.decls) {
            FE::AST::LeftValExpr* lval = dynamic_cast<FE::AST::LeftValExpr*>(decl->lval);
            DataType type = convert(node.type);
            
            bool isArray = false;
            std::vector<int> dims;
            if (lval->indices) {
                for (auto* expr : *lval->indices) {
                    if (expr->attr.val.isConstexpr) {
                        dims.push_back(expr->attr.val.getInt());
                        isArray = true;
                    }
                }
            }
            
            size_t stackReg = getNewRegId();
            AllocaInst* alloca = nullptr;
            if (isArray) {
                alloca = createAllocaInst(type, stackReg, dims);
            } else {
                alloca = createAllocaInst(type, stackReg);
            }
            // Insert alloca to entry block to avoid stack overflow in loops
            insertToEntry(alloca);
            name2reg.addSymbol(lval->entry, stackReg);
            
            // Store array dims for GEP generation
            FE::AST::VarAttr attr;
            attr.type = node.type;
            attr.arrayDims = dims;
            reg2attr[stackReg] = attr;
            
            if (decl->init) {
                if (!isArray) {
                    FE::AST::Initializer* simpleInit = dynamic_cast<FE::AST::Initializer*>(decl->init);
                    if (simpleInit && simpleInit->init_val) {
                        dispatch(simpleInit->init_val, m);
                        size_t valReg = getMaxReg();
                        
                        // Infer actual value type from last instruction
                        DataType valType = convert(simpleInit->init_val->attr.val.value.type);
                        if (!curBlock->insts.empty()) {
                            auto* lastInst = curBlock->insts.back();
                            if (auto* loadInst = dynamic_cast<LoadInst*>(lastInst)) {
                                valType = loadInst->dt;
                            } else if (auto* arithInst = dynamic_cast<ArithmeticInst*>(lastInst)) {
                                valType = arithInst->dt;
                            } else if (auto* icmpInst = dynamic_cast<IcmpInst*>(lastInst)) {
                                valType = DataType::I1;
                            } else if (auto* fcmpInst = dynamic_cast<FcmpInst*>(lastInst)) {
                                valType = DataType::I1;
                            }
                        }
                        
                        // PTR type should use I32 for storage
                        if (type == DataType::PTR) type = DataType::I32;
                        if (valType == DataType::PTR) valType = DataType::I32;
                        
                        if (valType != type) {
                            auto insts = createTypeConvertInst(valType, type, valReg);
                            for (auto* inst : insts) insert(inst);
                            valReg = getMaxReg();
                        }
                        insert(createStoreInst(type, valReg, getRegOperand(stackReg)));
                    }
                } else {
                    // Array initialization - for now just leave arrays uninitialized
                    // The test cases might expect zero-initialization or explicit initialization
                    // TODO: Implement full array initialization with init lists
                }
            }
        }
    }
}  // namespace ME

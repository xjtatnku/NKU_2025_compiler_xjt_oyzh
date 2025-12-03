#include <middleend/visitor/codegen/ast_codegen.h>

namespace ME
{
    void ASTCodeGen::visit(FE::AST::ExprStmt& node, Module* m)
    {
        if (!node.expr) return;
        dispatch(node.expr, m);
    }

    void ASTCodeGen::visit(FE::AST::FuncDeclStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成函数定义 IR（形参、入口/结束基本块、返回补丁）
        // 设置函数返回类型与参数寄存器，创建基本块骨架，并生成函数体
        
        DataType retType = convert(node.retType);
        std::string name = node.entry->getName();
        
        // Create FuncDefInst
        FuncDefInst* funcDef = new FuncDefInst(retType, name);
        
        // Create Function
        Function* func = new Function(funcDef);
        m->functions.push_back(func);
        enterFunc(func);
        
        // Clear paramPtrTab for this function
        paramPtrTab.clear();
        
        // Prepare args
        if (node.params) {
            for (auto* param : *node.params) {
                DataType paramType = convert(param->type);
                // If array dimension exists, it's a pointer
                if (param->dims && !param->dims->empty()) {
                    // Determine pointer type based on element type
                    if (paramType == DataType::F32) {
                        paramType = DataType::F32_PTR;  // float array -> float*
                    } else {
                        paramType = DataType::PTR;       // int array -> i32*
                    }
                }
                
                size_t argReg = getNewRegId();
                funcDef->argRegs.push_back({paramType, getRegOperand(argReg)});
            }
        }
        
        // Create Entry Block
        entryBlock = createBlock();
        enterBlock(entryBlock);
        
        // Scope for args
        name2reg.enterScope();
        
        if (node.params) {
            int argIdx = 0;
            for (auto* param : *node.params) {
                DataType paramType = funcDef->argRegs[argIdx].first;
                Operand* argOp = funcDef->argRegs[argIdx].second;
                size_t argReg = dynamic_cast<RegOperand*>(argOp)->getRegNum();
                
                if (paramType == DataType::PTR || paramType == DataType::F32_PTR) {
                    // Pointer parameters: use directly without alloca
                    name2reg.addSymbol(param->entry, argReg);
                    paramPtrTab[argReg] = true;
                    
                    // Store parameter attributes (element type + remaining array dims)
                    // For declarations like int b[][59], param->dims contains:
                    //   [ nullptr, const 59 ]
                    // We skip the first (decayed) dimension and record the rest (e.g. [59]),
                    // so that GEP on the pointer can still know inner dimensions.
                    FE::AST::VarAttr attr;
                    attr.type = param->type;  // base element type (int / float)
                    if (param->dims && !param->dims->empty()) {
                        for (size_t di = 0; di < param->dims->size(); ++di) {
                            // Skip the first (possibly omitted) dimension – it represents the decayed level.
                            if (di == 0) continue;
                            auto* dimExpr = (*param->dims)[di];
                            if (dimExpr && dimExpr->attr.val.isConstexpr) {
                                attr.arrayDims.push_back(dimExpr->attr.val.getInt());
                            }
                        }
                    }
                    reg2attr[argReg] = attr;
                } else {
                    // Scalar parameters: alloca + store (make them mutable)
                    size_t stackReg = getNewRegId();
                    AllocaInst* alloca = createAllocaInst(paramType, stackReg);
                    insertToEntry(alloca);
                    
                    StoreInst* store = createStoreInst(paramType, argReg, getRegOperand(stackReg));
                    insert(store);
                    
                    name2reg.addSymbol(param->entry, stackReg);
                }
                
                argIdx++;
            }
        }
        
        // Visit Body
        if (node.body) {
            dispatch(node.body, m);
        }
        
        // Exit Scope
        name2reg.exitScope();
        
        // Check Terminator
        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()) {
            if (retType == DataType::VOID) {
                insert(createRetInst());
            } else {
                if (name == "main" && retType == DataType::I32) {
                    insert(createRetInst(0));
                } else if (retType == DataType::I32) {
                    insert(createRetInst(0));
                } else if (retType == DataType::F32) {
                    insert(createRetInst(0.0f));
                } else {
                    insert(createRetInst(DataType::I32, getNewRegId())); // Fallback
                }
            }
        }
        
        exitFunc();
    }

    void ASTCodeGen::visit(FE::AST::VarDeclStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成变量声明语句 IR（局部变量分配、初始化）
        if (node.decl) {
            visit(*node.decl, m);
        }
    }

    void ASTCodeGen::visit(FE::AST::BlockStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成语句块 IR（作用域管理，顺序生成子语句）
        name2reg.enterScope();
        if (node.stmts) {
            for (auto* stmt : *node.stmts) {
                if (!stmt) continue;
                
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

                if (!curBlock->insts.empty() && curBlock->insts.back()->isTerminator()) break;
            }
        }
        name2reg.exitScope();
    }

    void ASTCodeGen::visit(FE::AST::ReturnStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 return 语句 IR（可选返回值与类型转换）
        if (node.retExpr) {
            dispatch(node.retExpr, m);
            size_t exprReg = getMaxReg();
            
            DataType retType = curFunc->funcDef->retType;
            
            // Infer actual expr type from last instruction
            DataType exprType = convert(node.retExpr->attr.val.value.type);
            if (!curBlock->insts.empty()) {
                auto* lastInst = curBlock->insts.back();
                if (auto* loadInst = dynamic_cast<LoadInst*>(lastInst)) {
                    exprType = loadInst->dt;
                } else if (auto* arithInst = dynamic_cast<ArithmeticInst*>(lastInst)) {
                    exprType = arithInst->dt;
                } else if (auto* icmpInst = dynamic_cast<IcmpInst*>(lastInst)) {
                    exprType = DataType::I1;
                } else if (auto* fcmpInst = dynamic_cast<FcmpInst*>(lastInst)) {
                    exprType = DataType::I1;
                }
            }
            
            if (retType != exprType) {
                auto convInsts = createTypeConvertInst(exprType, retType, exprReg);
                for (auto* inst : convInsts) insert(inst);
                exprReg = getMaxReg();
            }
            insert(createRetInst(retType, exprReg));
        } else {
            insert(createRetInst());
        }
    }

    void ASTCodeGen::visit(FE::AST::WhileStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 while 循环 IR（条件块、循环体与结束块、循环标签）
        Block* condBB = createBlock();
        Block* bodyBB = createBlock();
        Block* endBB = createBlock();
        
        size_t oldStart = curFunc->loopStartLabel;
        size_t oldEnd = curFunc->loopEndLabel;
        curFunc->loopStartLabel = condBB->blockId;
        curFunc->loopEndLabel = endBB->blockId;
        
        insert(createBranchInst(condBB->blockId));
        
        // Cond
        enterBlock(condBB);
        if (node.cond) {
            dispatch(node.cond, m);
            size_t condReg = getMaxReg();
            
            // Get actual type from last instruction (more reliable than attr)
            DataType actualType = DataType::I1;
            if (!curBlock->insts.empty()) {
                auto* lastInst = curBlock->insts.back();
                // Try to infer type from instruction
                if (auto* loadInst = dynamic_cast<LoadInst*>(lastInst)) {
                    actualType = loadInst->dt;
                } else if (auto* icmpInst = dynamic_cast<IcmpInst*>(lastInst)) {
                    actualType = DataType::I1;
                } else if (auto* fcmpInst = dynamic_cast<FcmpInst*>(lastInst)) {
                    actualType = DataType::I1;
                } else if (auto* arithInst = dynamic_cast<ArithmeticInst*>(lastInst)) {
                    actualType = arithInst->dt;
                } else {
                    // Fallback to attr type
                    actualType = convert(node.cond->attr.val.value.type);
                }
            } else {
                actualType = convert(node.cond->attr.val.value.type);
            }
            
            // Convert non-bool types to bool for branching
            if (actualType == DataType::I32) {
                size_t newReg = getNewRegId();
                insert(createIcmpInst_ImmeRight(ICmpOp::NE, condReg, 0, newReg));
                condReg = newReg;
            } else if (actualType == DataType::F32) {
                size_t newReg = getNewRegId();
                insert(createFcmpInst_ImmeRight(FCmpOp::ONE, condReg, 0.0f, newReg));
                condReg = newReg;
            }
            insert(createBranchInst(condReg, bodyBB->blockId, endBB->blockId));
        } else {
            // No cond -> true
            insert(createBranchInst(bodyBB->blockId));
        }
        
        // Body
        enterBlock(bodyBB);
        if (node.body) {
            dispatch(node.body, m);
        }
        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()) {
            insert(createBranchInst(condBB->blockId));
        }
        
        // End
        enterBlock(endBB);
        
        curFunc->loopStartLabel = oldStart;
        curFunc->loopEndLabel = oldEnd;
    }

    void ASTCodeGen::visit(FE::AST::IfStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 if/else IR（then/else/end 基本块与条件分支）
        Block* thenBB = createBlock();
        Block* endBB = createBlock();
        Block* elseBB = node.elseStmt ? createBlock() : nullptr;
        
        if (node.cond) {
            dispatch(node.cond, m);
            size_t condReg = getMaxReg();
            
            // Get actual type from last instruction
            DataType actualType = DataType::I1;
            if (!curBlock->insts.empty()) {
                auto* lastInst = curBlock->insts.back();
                if (auto* loadInst = dynamic_cast<LoadInst*>(lastInst)) {
                    actualType = loadInst->dt;
                } else if (auto* icmpInst = dynamic_cast<IcmpInst*>(lastInst)) {
                    actualType = DataType::I1;
                } else if (auto* fcmpInst = dynamic_cast<FcmpInst*>(lastInst)) {
                    actualType = DataType::I1;
                } else if (auto* arithInst = dynamic_cast<ArithmeticInst*>(lastInst)) {
                    actualType = arithInst->dt;
                } else {
                    actualType = convert(node.cond->attr.val.value.type);
                }
            } else {
                actualType = convert(node.cond->attr.val.value.type);
            }
            
            // Convert non-bool types to bool for branching
            if (actualType == DataType::I32) {
                size_t newReg = getNewRegId();
                insert(createIcmpInst_ImmeRight(ICmpOp::NE, condReg, 0, newReg));
                condReg = newReg;
            } else if (actualType == DataType::F32) {
                size_t newReg = getNewRegId();
                insert(createFcmpInst_ImmeRight(FCmpOp::ONE, condReg, 0.0f, newReg));
                condReg = newReg;
            }
            
            insert(createBranchInst(condReg, thenBB->blockId, elseBB ? elseBB->blockId : endBB->blockId));
        }
        
        // Then
        enterBlock(thenBB);
        if (node.thenStmt) dispatch(node.thenStmt, m);
        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()) insert(createBranchInst(endBB->blockId));
        
        // Else
        if (elseBB) {
            enterBlock(elseBB);
            if (node.elseStmt) dispatch(node.elseStmt, m);
            if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()) insert(createBranchInst(endBB->blockId));
        }
        
        enterBlock(endBB);
    }

    void ASTCodeGen::visit(FE::AST::BreakStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 break 的无条件跳转至循环结束块
        (void)node; (void)m;
        insert(createBranchInst(curFunc->loopEndLabel));
    }

    void ASTCodeGen::visit(FE::AST::ContinueStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 continue 的无条件跳转至循环步进/条件块
        (void)node; (void)m;
        insert(createBranchInst(curFunc->loopStartLabel));
    }

    void ASTCodeGen::visit(FE::AST::ForStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 for 循环 IR（init/cond/body/step 基本块与循环标签）
        // For loop structure:
        // Init -> Cond -> Body -> Step -> Cond -> End
        
        name2reg.enterScope(); // Init can declare var
        
        if (node.init) dispatch(node.init, m);
        
        Block* condBB = createBlock();
        Block* bodyBB = createBlock();
        Block* stepBB = createBlock(); // step block acts as loop start for continue
        Block* endBB = createBlock();
        
        size_t oldStart = curFunc->loopStartLabel;
        size_t oldEnd = curFunc->loopEndLabel;
        curFunc->loopStartLabel = stepBB->blockId;
        curFunc->loopEndLabel = endBB->blockId;
        
        insert(createBranchInst(condBB->blockId));
        
        // Cond
        enterBlock(condBB);
        if (node.cond) {
            dispatch(node.cond, m);
            DataType condType = convert(node.cond->attr.val.value.type);
            size_t condReg = getMaxReg();
            if (condType == DataType::I32) {
                size_t newReg = getNewRegId();
                insert(createIcmpInst_ImmeRight(ICmpOp::NE, condReg, 0, newReg));
                condReg = newReg;
            } else if (condType == DataType::F32) {
                size_t newReg = getNewRegId();
                insert(createFcmpInst_ImmeRight(FCmpOp::ONE, condReg, 0.0f, newReg));
                condReg = newReg;
            }
            insert(createBranchInst(condReg, bodyBB->blockId, endBB->blockId));
        } else {
            insert(createBranchInst(bodyBB->blockId));
        }
        
        // Body
        enterBlock(bodyBB);
        if (node.body) dispatch(node.body, m);
        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()) insert(createBranchInst(stepBB->blockId));
        
        // Step
        enterBlock(stepBB);
        if (node.step) dispatch(node.step, m);
        insert(createBranchInst(condBB->blockId));
        
        // End
        enterBlock(endBB);
        
        curFunc->loopStartLabel = oldStart;
        curFunc->loopEndLabel = oldEnd;
        
        name2reg.exitScope();
    }
}  // namespace ME

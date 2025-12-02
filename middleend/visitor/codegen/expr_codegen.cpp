#include <middleend/visitor/codegen/ast_codegen.h>

namespace ME
{
    void ASTCodeGen::visit(FE::AST::LeftValExpr& node, Module* m)
    {
        // R-value context: compute address, load value.
        Operand* addrOp = nullptr;
        size_t reg = name2reg.getReg(node.entry);
        
        if (reg != static_cast<size_t>(-1)) {
            Operand* base = getRegOperand(reg);
            // For pointer parameters, the register already holds the pointer value
            // No need to load it
            
            if (node.indices && !node.indices->empty()) {
                std::vector<Operand*> idxOps;
                if (!paramPtrTab[reg]) idxOps.push_back(getImmeI32Operand(0));
                for (auto* index : *node.indices) {
                    dispatch(index, m);
                    size_t idxReg = getMaxReg();
                    DataType idxType = convert(index->attr.val.value.type);
                    // GEP requires i32 indices, convert i1 to i32 if needed
                    if (idxType == DataType::I1) {
                        size_t i32Reg = getNewRegId();
                        insert(createZextInst(idxReg, i32Reg, 1, 32));
                        idxReg = i32Reg;
                    }
                    idxOps.push_back(getRegOperand(idxReg));
                }
                size_t resReg = getNewRegId();
                std::vector<int> dims;
                DataType elemType = DataType::I32;
                if (reg2attr.find(reg) != reg2attr.end()) {
                    dims = reg2attr[reg].arrayDims;
                    elemType = convert(reg2attr[reg].type);
                }
                insert(createGEP_I32Inst(elemType, base, dims, idxOps, resReg));
                addrOp = getRegOperand(resReg);
            } else {
                addrOp = base;
            }
        } else {
            addrOp = getGlobalOperand(node.entry->getName());
            if (node.indices && !node.indices->empty()) {
                std::vector<Operand*> idxOps;
                idxOps.push_back(getImmeI32Operand(0));
                for (auto* index : *node.indices) {
                    dispatch(index, m);
                    size_t idxReg = getMaxReg();
                    DataType idxType = convert(index->attr.val.value.type);
                    // GEP requires i32 indices, convert i1 to i32 if needed
                    if (idxType == DataType::I1) {
                        size_t i32Reg = getNewRegId();
                        insert(createZextInst(idxReg, i32Reg, 1, 32));
                        idxReg = i32Reg;
                    }
                    idxOps.push_back(getRegOperand(idxReg));
                }
                size_t resReg = getNewRegId();
                std::vector<int> dims;
                DataType elemType = DataType::I32;
                if (glbSymbols.find(node.entry) != glbSymbols.end()) {
                    dims = glbSymbols.at(node.entry).arrayDims;
                    elemType = convert(glbSymbols.at(node.entry).type);
                }
                insert(createGEP_I32Inst(elemType, addrOp, dims, idxOps, resReg));
                addrOp = getRegOperand(resReg);
            }
        }
        
        DataType type = convert(node.attr.val.value.type);
        
        // Determine if we need to load or just return the address
        // For arrays:
        //   - No indices or partial indices: return pointer to sub-array (no load)
        //   - Full indices (indices.size() == dims.size()): load the element
        // For scalars: load the value
        
        bool needsLoad = true;
        size_t numIndices = (node.indices ? node.indices->size() : 0);
        size_t numDims = 0;
        DataType elemType = DataType::I32;
        
        if (reg != static_cast<size_t>(-1) && reg2attr.find(reg) != reg2attr.end()) {
            numDims = reg2attr[reg].arrayDims.size();
            elemType = convert(reg2attr[reg].type);
        } else if (glbSymbols.find(node.entry) != glbSymbols.end()) {
            numDims = glbSymbols.at(node.entry).arrayDims.size();
            elemType = convert(glbSymbols.at(node.entry).type);
        }
        
        if (type == DataType::PTR && numIndices < numDims) {
            // Partial array indexing: return pointer to sub-array, no load
            needsLoad = false;
        }
        
        size_t resReg;
        
        if (type == DataType::PTR && numIndices == 0) {
            // Array variable without index: return pointer to first element
            if (name2reg.getReg(node.entry) != static_cast<size_t>(-1)) {
                resReg = getNewRegId();
                std::vector<int> dims;
                DataType elemType = DataType::I32;
                if (reg2attr.find(reg) != reg2attr.end()) {
                    dims = reg2attr[reg].arrayDims;
                    elemType = convert(reg2attr[reg].type);
                }
                if (paramPtrTab[reg]) {
                    // Array parameter: already a pointer, just use it
                    insert(createGEP_I32Inst(elemType, addrOp, {}, {getImmeI32Operand(0)}, resReg));
                } else {
                    insert(createGEP_I32Inst(elemType, addrOp, dims, {getImmeI32Operand(0), getImmeI32Operand(0)}, resReg));
                }
            } else {
                // Global array
                resReg = getNewRegId();
                std::vector<int> dims;
                DataType elemType = DataType::I32;
                if (glbSymbols.find(node.entry) != glbSymbols.end()) {
                    dims = glbSymbols.at(node.entry).arrayDims;
                    elemType = convert(glbSymbols.at(node.entry).type);
                }
                insert(createGEP_I32Inst(elemType, addrOp, dims, {getImmeI32Operand(0), getImmeI32Operand(0)}, resReg));
            }
        } else if (needsLoad) {
            // Scalar or fully-indexed array element: load the value
            resReg = getNewRegId();
            DataType loadType = type;
            
            // For array element access with indices, use element type
            // This includes both array[index] and pointer[index]
            if (numIndices > 0) {
                loadType = elemType;
            } else if (type == DataType::PTR || type == DataType::F32_PTR) {
                loadType = DataType::I32;  // Default for pointer types without indexing
            }
            insert(createLoadInst(loadType, addrOp, resReg));
        } else {
            // Partial array index: addrOp points to sub-array (e.g., c[0] -> [4 x i32]*)
            // Need to get pointer to first element (e.g., -> i32*)
            // Add GEP with indices [0, 0] to get address of first element
            resReg = getNewRegId();
            std::vector<int> remainingDims;
            
            // Get the remaining dimensions after partial indexing
            if (reg != static_cast<size_t>(-1) && reg2attr.find(reg) != reg2attr.end()) {
                const auto& fullDims = reg2attr[reg].arrayDims;
                remainingDims = std::vector<int>(fullDims.begin() + numIndices, fullDims.end());
                elemType = convert(reg2attr[reg].type);
            } else if (glbSymbols.find(node.entry) != glbSymbols.end()) {
                const auto& fullDims = glbSymbols.at(node.entry).arrayDims;
                remainingDims = std::vector<int>(fullDims.begin() + numIndices, fullDims.end());
                elemType = convert(glbSymbols.at(node.entry).type);
            }
            
            // Generate GEP to get pointer to first element of sub-array
            insert(createGEP_I32Inst(elemType, addrOp, remainingDims, {getImmeI32Operand(0), getImmeI32Operand(0)}, resReg));
        }
        
        lval2ptr[&node] = addrOp;
    }

    void ASTCodeGen::visit(FE::AST::LiteralExpr& node, Module* m)
    {
        (void)m;

        size_t reg = getNewRegId();
        switch (node.literal.type->getBaseType())
        {
            case FE::AST::Type_t::INT:
            case FE::AST::Type_t::LL:  // treat as I32
            {
                int             val  = node.literal.getInt();
                ArithmeticInst* inst = createArithmeticI32Inst_ImmeAll(Operator::ADD, val, 0, reg);  // reg = val + 0
                insert(inst);
                break;
            }
            case FE::AST::Type_t::FLOAT:
            {
                float           val  = node.literal.getFloat();
                ArithmeticInst* inst = createArithmeticF32Inst_ImmeAll(Operator::FADD, val, 0, reg);  // reg = val + 0
                insert(inst);
                break;
            }
            default: ERROR("Unsupported literal type");
        }
    }

    void ASTCodeGen::visit(FE::AST::UnaryExpr& node, Module* m)
    {
        handleUnaryCalc(*node.expr, node.op, curBlock, m);
    }

    void ASTCodeGen::handleAssign(FE::AST::LeftValExpr& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        dispatch(&rhs, m);
        size_t rhsReg = getMaxReg();
        
        // Infer actual rhs type from last instruction
        DataType rhsType = convert(rhs.attr.val.value.type);
        if (!curBlock->insts.empty()) {
            auto* lastInst = curBlock->insts.back();
            if (auto* loadInst = dynamic_cast<LoadInst*>(lastInst)) {
                rhsType = loadInst->dt;
            } else if (auto* arithInst = dynamic_cast<ArithmeticInst*>(lastInst)) {
                rhsType = arithInst->dt;
            } else if (auto* icmpInst = dynamic_cast<IcmpInst*>(lastInst)) {
                rhsType = DataType::I1;
            } else if (auto* fcmpInst = dynamic_cast<FcmpInst*>(lastInst)) {
                rhsType = DataType::I1;
            }
        }
        
        // Compute Address (Logic duplicated from visit(LeftValExpr) but without Load)
        Operand* addrOp = nullptr;
        size_t reg = name2reg.getReg(lhs.entry);
        
        if (reg != static_cast<size_t>(-1)) {
            Operand* base = getRegOperand(reg);
            // For pointer parameters, the register already holds the pointer value
            
            if (lhs.indices && !lhs.indices->empty()) {
                std::vector<Operand*> idxOps;
                if (!paramPtrTab[reg]) idxOps.push_back(getImmeI32Operand(0));
                for (auto* index : *lhs.indices) {
                    dispatch(index, m);
                    size_t idxReg = getMaxReg();
                    DataType idxType = convert(index->attr.val.value.type);
                    // GEP requires i32 indices, convert i1 to i32 if needed
                    if (idxType == DataType::I1) {
                        size_t i32Reg = getNewRegId();
                        insert(createZextInst(idxReg, i32Reg, 1, 32));
                        idxReg = i32Reg;
                    }
                    idxOps.push_back(getRegOperand(idxReg));
                }
                size_t resReg = getNewRegId();
                std::vector<int> dims;
                DataType elemType = DataType::I32;
                if (reg2attr.find(reg) != reg2attr.end()) {
                    dims = reg2attr[reg].arrayDims;
                    elemType = convert(reg2attr[reg].type);
                }
                insert(createGEP_I32Inst(elemType, base, dims, idxOps, resReg));
                addrOp = getRegOperand(resReg);
            } else {
                addrOp = base;
            }
        } else {
            addrOp = getGlobalOperand(lhs.entry->getName());
            if (lhs.indices && !lhs.indices->empty()) {
                std::vector<Operand*> idxOps;
                idxOps.push_back(getImmeI32Operand(0));
                for (auto* index : *lhs.indices) {
                    dispatch(index, m);
                    size_t idxReg = getMaxReg();
                    DataType idxType = convert(index->attr.val.value.type);
                    // GEP requires i32 indices, convert i1 to i32 if needed
                    if (idxType == DataType::I1) {
                        size_t i32Reg = getNewRegId();
                        insert(createZextInst(idxReg, i32Reg, 1, 32));
                        idxReg = i32Reg;
                    }
                    idxOps.push_back(getRegOperand(idxReg));
                }
                size_t resReg = getNewRegId();
                std::vector<int> dims;
                DataType elemType = DataType::I32;
                if (glbSymbols.find(lhs.entry) != glbSymbols.end()) {
                    dims = glbSymbols.at(lhs.entry).arrayDims;
                    elemType = convert(glbSymbols.at(lhs.entry).type);
                }
                insert(createGEP_I32Inst(elemType, addrOp, dims, idxOps, resReg));
                addrOp = getRegOperand(resReg);
            }
        }
        
        DataType lhsType = convert(lhs.attr.val.value.type);
        
        // Determine actual storage type
        // If we have indices (array element assignment), use element type
        size_t numIndices = (lhs.indices ? lhs.indices->size() : 0);
        if (numIndices > 0) {
            // Array element assignment: use element type
            DataType elemType = DataType::I32;
            if (reg != static_cast<size_t>(-1) && reg2attr.find(reg) != reg2attr.end()) {
                elemType = convert(reg2attr[reg].type);
            } else if (glbSymbols.find(lhs.entry) != glbSymbols.end()) {
                elemType = convert(glbSymbols.at(lhs.entry).type);
            }
            lhsType = elemType;
        } else if (lhsType == DataType::PTR) {
            lhsType = DataType::I32;
        }
        
        // Convert rhs pointer type to I32 if needed
        if (rhsType == DataType::PTR) rhsType = DataType::I32;
        
        if (lhsType != rhsType) {
            auto insts = createTypeConvertInst(rhsType, lhsType, rhsReg);
            for (auto* inst : insts) insert(inst);
            rhsReg = getMaxReg();
        }
        insert(createStoreInst(lhsType, rhsReg, addrOp));
    }

    void ASTCodeGen::handleLogicalAnd(
        FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        (void)node;
        size_t resPtr = getNewRegId();
        insertToEntry(createAllocaInst(DataType::I32, resPtr));
        
        Block* rhsBB = createBlock();
        Block* trueBB = createBlock();
        Block* falseBB = createBlock();
        Block* endBB = createBlock();
        
        dispatch(&lhs, m);
        size_t lhsReg = getMaxReg();
        DataType lhsType = convert(lhs.attr.val.value.type);
        if (lhsType != DataType::I1) {
            size_t i1Reg = getNewRegId();
            if (lhsType == DataType::I32) insert(createIcmpInst_ImmeRight(ICmpOp::NE, lhsReg, 0, i1Reg));
            else insert(createFcmpInst_ImmeRight(FCmpOp::ONE, lhsReg, 0.0f, i1Reg));
            lhsReg = i1Reg;
        }
        insert(createBranchInst(lhsReg, rhsBB->blockId, falseBB->blockId));
        
        enterBlock(rhsBB);
        dispatch(&rhs, m);
        size_t rhsReg = getMaxReg();
        DataType rhsType = convert(rhs.attr.val.value.type);
        if (rhsType != DataType::I1) {
            size_t i1Reg = getNewRegId();
            if (rhsType == DataType::I32) insert(createIcmpInst_ImmeRight(ICmpOp::NE, rhsReg, 0, i1Reg));
            else insert(createFcmpInst_ImmeRight(FCmpOp::ONE, rhsReg, 0.0f, i1Reg));
            rhsReg = i1Reg;
        }
        insert(createBranchInst(rhsReg, trueBB->blockId, falseBB->blockId));
        
        enterBlock(trueBB);
        insert(createStoreInst(DataType::I32, getImmeI32Operand(1), getRegOperand(resPtr)));
        insert(createBranchInst(endBB->blockId));
        
        enterBlock(falseBB);
        insert(createStoreInst(DataType::I32, getImmeI32Operand(0), getRegOperand(resPtr)));
        insert(createBranchInst(endBB->blockId));
        
        enterBlock(endBB);
        size_t resReg = getNewRegId();
        insert(createLoadInst(DataType::I32, getRegOperand(resPtr), resReg));
        // Convert i32 to i1 for conditional branches
        size_t i1Reg = getNewRegId();
        insert(createIcmpInst_ImmeRight(ICmpOp::NE, resReg, 0, i1Reg));
    }

    void ASTCodeGen::handleLogicalOr(
        FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        (void)node;
        size_t resPtr = getNewRegId();
        insertToEntry(createAllocaInst(DataType::I32, resPtr));
        
        Block* rhsBB = createBlock();
        Block* trueBB = createBlock();
        Block* falseBB = createBlock();
        Block* endBB = createBlock();
        
        dispatch(&lhs, m);
        size_t lhsReg = getMaxReg();
        DataType lhsType = convert(lhs.attr.val.value.type);
        if (lhsType != DataType::I1) {
            size_t i1Reg = getNewRegId();
            if (lhsType == DataType::I32) insert(createIcmpInst_ImmeRight(ICmpOp::NE, lhsReg, 0, i1Reg));
            else insert(createFcmpInst_ImmeRight(FCmpOp::ONE, lhsReg, 0.0f, i1Reg));
            lhsReg = i1Reg;
        }
        insert(createBranchInst(lhsReg, trueBB->blockId, rhsBB->blockId));
        
        enterBlock(rhsBB);
        dispatch(&rhs, m);
        size_t rhsReg = getMaxReg();
        DataType rhsType = convert(rhs.attr.val.value.type);
        if (rhsType != DataType::I1) {
            size_t i1Reg = getNewRegId();
            if (rhsType == DataType::I32) insert(createIcmpInst_ImmeRight(ICmpOp::NE, rhsReg, 0, i1Reg));
            else insert(createFcmpInst_ImmeRight(FCmpOp::ONE, rhsReg, 0.0f, i1Reg));
            rhsReg = i1Reg;
        }
        insert(createBranchInst(rhsReg, trueBB->blockId, falseBB->blockId));
        
        enterBlock(trueBB);
        insert(createStoreInst(DataType::I32, getImmeI32Operand(1), getRegOperand(resPtr)));
        insert(createBranchInst(endBB->blockId));
        
        enterBlock(falseBB);
        insert(createStoreInst(DataType::I32, getImmeI32Operand(0), getRegOperand(resPtr)));
        insert(createBranchInst(endBB->blockId));
        
        enterBlock(endBB);
        size_t resReg = getNewRegId();
        insert(createLoadInst(DataType::I32, getRegOperand(resPtr), resReg));
        // Convert i32 to i1 for conditional branches
        size_t i1Reg = getNewRegId();
        insert(createIcmpInst_ImmeRight(ICmpOp::NE, resReg, 0, i1Reg));
    }

    void ASTCodeGen::visit(FE::AST::BinaryExpr& node, Module* m)
    {
        if (node.op == FE::AST::Operator::ASSIGN) {
            FE::AST::LeftValExpr* lhs = dynamic_cast<FE::AST::LeftValExpr*>(node.lhs);
            handleAssign(*lhs, *node.rhs, m);
        } else if (node.op == FE::AST::Operator::AND) {
            handleLogicalAnd(node, *node.lhs, *node.rhs, m);
        } else if (node.op == FE::AST::Operator::OR) {
            handleLogicalOr(node, *node.lhs, *node.rhs, m);
        } else {
            handleBinaryCalc(*node.lhs, *node.rhs, node.op, curBlock, m);
        }
    }

    void ASTCodeGen::visit(FE::AST::CallExpr& node, Module* m)
    {
        std::string name = node.func->getName();
        DataType retType = convert(node.attr.val.value.type);
        
        std::vector<DataType> argTypes;
        for (auto* fd : m->funcDecls) {
            if (fd->funcName == name) {
                argTypes = fd->argTypes;
                break;
            }
        }
        if (argTypes.empty()) {
            for (auto* f : m->functions) {
                if (f->funcDef->funcName == name) {
                    for (auto& p : f->funcDef->argRegs) argTypes.push_back(p.first);
                    break;
                }
            }
        }
        
        CallInst::argList args;
        if (node.args) {
            for (size_t i = 0; i < node.args->size(); ++i) {
                FE::AST::ExprNode* arg = (*node.args)[i];
                dispatch(arg, m);
                size_t argReg = getMaxReg();
                DataType argType = convert(arg->attr.val.value.type);
                
                // Special handling for array pointers (PTR type)
                // Need to determine if it's float* or i32* based on array element type
                if (argType == DataType::PTR) {
                    // Check if it's a LeftValExpr (array)
                    if (auto* lval = dynamic_cast<FE::AST::LeftValExpr*>(arg)) {
                        // Get the actual element type of the array
                        size_t reg = name2reg.getReg(lval->entry);
                        if (reg != static_cast<size_t>(-1) && reg2attr.find(reg) != reg2attr.end()) {
                            DataType elemType = convert(reg2attr[reg].type);
                            if (elemType == DataType::F32) {
                                argType = DataType::F32_PTR;  // float* for float arrays
                            }
                            // else keep PTR (i32*) for int arrays
                        } else if (glbSymbols.find(lval->entry) != glbSymbols.end()) {
                            DataType elemType = convert(glbSymbols.at(lval->entry).type);
                            if (elemType == DataType::F32) {
                                argType = DataType::F32_PTR;
                            }
                        }
                    }
                }
                
                if (i < argTypes.size()) {
                    DataType expected = argTypes[i];
                    // Check if types match or are compatible
                    bool compatible = (argType == expected);
                    // Pointer types are compatible with PTR
                    if ((expected == DataType::PTR && (argType == DataType::PTR || argType == DataType::F32_PTR)) ||
                        (argType == DataType::PTR && (expected == DataType::PTR || expected == DataType::F32_PTR)) ||
                        (expected == DataType::F32_PTR && argType == DataType::F32_PTR)) {
                        compatible = true;
                    }
                    
                    if (!compatible) {
                        auto insts = createTypeConvertInst(argType, expected, argReg);
                        for (auto* inst : insts) insert(inst);
                        argReg = getMaxReg();
                        argType = expected;
                    }
                }
                args.push_back({argType, getRegOperand(argReg)});
            }
        }
        
        if (retType == DataType::VOID) {
            insert(createCallInst(retType, name, args));
        } else {
            size_t resReg = getNewRegId();
            insert(createCallInst(retType, name, args, resReg));
        }
    }

    void ASTCodeGen::visit(FE::AST::CommaExpr& node, Module* m)
    {
        if (node.exprs) {
            for (auto* expr : *node.exprs) {
                dispatch(expr, m);
            }
        }
    }
}  // namespace ME

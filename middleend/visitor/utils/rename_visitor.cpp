#include <middleend/visitor/utils/rename_visitor.h>
#include <middleend/module/ir_operand.h>

namespace ME
{
    void renameReg(Operand*& operand, RegMap& renameMap)
    {
        if (!operand || operand->getType() != OperandType::REG) return;

        RegOperand* regOp = static_cast<RegOperand*>(operand);
        auto        it    = renameMap.find(regOp->regNum);
        if (it == renameMap.end()) return;
        operand = getRegOperand(it->second);
    }

    void RegRename::visit(LoadInst& inst, RegMap& rm)
    {
        renameReg(inst.ptr, rm);
        renameReg(inst.res, rm);
    }

    void RegRename::visit(StoreInst& inst, RegMap& rm)
    {
        renameReg(inst.ptr, rm);
        renameReg(inst.val, rm);
    }

    void RegRename::visit(ArithmeticInst& inst, RegMap& rm)
    {
        renameReg(inst.lhs, rm);
        renameReg(inst.rhs, rm);
        renameReg(inst.res, rm);
    }

    void RegRename::visit(IcmpInst& inst, RegMap& rm)
    {
        renameReg(inst.lhs, rm);
        renameReg(inst.rhs, rm);
        renameReg(inst.res, rm);
    }

    void RegRename::visit(FcmpInst& inst, RegMap& rm)
    {
        renameReg(inst.lhs, rm);
        renameReg(inst.rhs, rm);
        renameReg(inst.res, rm);
    }

    void RegRename::visit(AllocaInst& inst, RegMap& rm) { renameReg(inst.res, rm); }

    void RegRename::visit(BrCondInst& inst, RegMap& rm) { renameReg(inst.cond, rm); }

    void RegRename::visit(BrUncondInst& inst, RegMap& rm)
    {
        (void)inst;
        (void)rm;
    }

    void RegRename::visit(GlbVarDeclInst& inst, RegMap& rm) { renameReg(inst.init, rm); }

    void RegRename::visit(CallInst& inst, RegMap& rm)
    {
        for (auto& arg : inst.args) renameReg(arg.second, rm);
        renameReg(inst.res, rm);
    }

    void RegRename::visit(FuncDeclInst& inst, RegMap& rm)
    {
        (void)inst;
        (void)rm;
    }

    void RegRename::visit(FuncDefInst& inst, RegMap& rm)
    {
        for (auto& arg : inst.argRegs) renameReg(arg.second, rm);
    }

    void RegRename::visit(RetInst& inst, RegMap& rm) { renameReg(inst.res, rm); }

    void RegRename::visit(GEPInst& inst, RegMap& rm)
    {
        renameReg(inst.basePtr, rm);
        renameReg(inst.res, rm);
        for (auto& idx : inst.idxs) renameReg(idx, rm);
    }

    void RegRename::visit(FP2SIInst& inst, RegMap& rm)
    {
        renameReg(inst.src, rm);
        renameReg(inst.dest, rm);
    }

    void RegRename::visit(SI2FPInst& inst, RegMap& rm)
    {
        renameReg(inst.src, rm);
        renameReg(inst.dest, rm);
    }

    void RegRename::visit(ZextInst& inst, RegMap& rm)
    {
        renameReg(inst.src, rm);
        renameReg(inst.dest, rm);
    }

    void RegRename::visit(PhiInst& inst, RegMap& rm)
    {
        renameReg(inst.res, rm);
        std::map<Operand*, Operand*> newIncomingVals;
        for (auto& [label, val] : inst.incomingVals)
        {
            Operand* newVal = val;
            renameReg(newVal, rm);
            newIncomingVals[label] = newVal;
        }
        inst.incomingVals = newIncomingVals;
    }
}  // namespace ME

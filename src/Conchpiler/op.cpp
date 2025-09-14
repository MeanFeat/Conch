#include "op.h"

void ConBaseOp::SetArgs(const vector<Operand> Args)
{
    assert(GetArgsCount() <= GetMaxArgs());
    this->Args = Args;
}

Operand ConBaseOp::GetReturn() const
{
    assert(HasReturn());
    return {};
}

const vector<Operand> &ConBaseOp::GetArgs() const
{
    return Args;
}


Operand ConContextualReturnOp::GetDstArg() const
{
    assert(GetArgsCount() > 0);
    return GetArgs().at(0);
}

vector<Operand> ConContextualReturnOp::GetSrcArg() const
{
    if (GetArgsCount() == 3)
    {
        return {GetArgs().at(1), GetArgs().at(2)};
    }
    return {GetDstArg(), GetArgs().at(1)};
}

void ConAddOp::Execute(vector<ConVariable*>& regs)
{
    const vector<Operand> SrcArg = GetSrcArg();
    int32 lhs = ReadOperand(SrcArg.at(0), regs);
    int32 rhs = ReadOperand(SrcArg.at(1), regs);
    Operand dst = GetDstArg();
    assert(dst.kind == OperandKind::Reg);
    regs[dst.value]->SetVal(lhs + rhs);
}

void ConMulOp::Execute(vector<ConVariable*>& regs)
{
    const vector<Operand> SrcArg = GetSrcArg();
    int32 lhs = ReadOperand(SrcArg.at(0), regs);
    int32 rhs = ReadOperand(SrcArg.at(1), regs);
    Operand dst = GetDstArg();
    assert(dst.kind == OperandKind::Reg);
    regs[dst.value]->SetVal(lhs * rhs);
}

void ConSubOp::Execute(vector<ConVariable*>& regs)
{
    const vector<Operand> SrcArg = GetSrcArg();
    int32 lhs = ReadOperand(SrcArg.at(0), regs);
    int32 rhs = ReadOperand(SrcArg.at(1), regs);
    Operand dst = GetDstArg();
    assert(dst.kind == OperandKind::Reg);
    regs[dst.value]->SetVal(lhs - rhs);
}

void ConSetOp::Execute(vector<ConVariable*>& regs)
{
    Operand dst = GetArgs().at(0);
    Operand src = GetArgs().at(1);
    assert(dst.kind == OperandKind::Reg);
    regs[dst.value]->SetVal(ReadOperand(src, regs));
}

void ConSwpOp::Execute(vector<ConVariable*>& regs)
{
    Operand dst = GetArgs().at(0);
    assert(dst.kind == OperandKind::Reg);
    ConVariableCached* var = dynamic_cast<ConVariableCached*>(regs[dst.value]);
    assert(var);
    var->Swap();
}




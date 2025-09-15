#include "line.h"

ConLine::~ConLine()
{
}

void ConLine::Execute()
{
    for (ConBaseOp* Op : Ops)
    {
        Op->Execute();
    }
}

void ConLine::UpdateCycleCount()
{
    ConCompilable::UpdateCycleCount();
    for (ConBaseOp* Op : Ops)
    {
        Op->UpdateCycleCount();
        AddCycles(Op->GetCycleCount());
    }
}

void ConLine::SetOps(const vector<ConBaseOp*>& Ops)
{
    this->Ops = Ops;
}

void ConLine::SetCondition(ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, int32 SkipCount)
{
    Condition = Op;
    Left = Lhs;
    Right = Rhs;
    Skip = SkipCount;
}

bool ConLine::EvaluateCondition() const
{
    switch (Condition)
    {
    case ConConditionOp::GTR:
        return Left->GetVal() > Right->GetVal();
    case ConConditionOp::LSR:
        return Left->GetVal() < Right->GetVal();
    default:
        return true;
    }
}

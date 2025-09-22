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
    UpdateCycleCount(0);
}

void ConLine::UpdateCycleCount(const int32 VarCount)
{
    ConCompilable::UpdateCycleCount();
    for (ConBaseOp* Op : Ops)
    {
        Op->UpdateCycleCount(VarCount);
        AddCycles(Op->GetCycleCount());
    }
}

void ConLine::SetOps(const vector<ConBaseOp*>& Ops)
{
    this->Ops = Ops;
}

void ConLine::SetCondition(ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, int32 SkipCount, bool bInvert)
{
    Condition = Op;
    Left = Lhs;
    Right = Rhs;
    Skip = SkipCount;
    Invert = bInvert;
}

bool ConLine::EvaluateCondition() const
{
    bool Result = true;
    switch (Condition)
    {
    case ConConditionOp::GTR:
        Result = Left->GetVal() > Right->GetVal();
        break;
    case ConConditionOp::LSR:
        Result = Left->GetVal() < Right->GetVal();
        break;
    case ConConditionOp::EQL:
        Result = Left->GetVal() == Right->GetVal();
        break;
    default:
        Result = true;
        break;
    }
    return Invert ? !Result : Result;
}

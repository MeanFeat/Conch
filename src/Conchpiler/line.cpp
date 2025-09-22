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
    switch (Kind)
    {
    case ConLineKind::Ops:
        for (ConBaseOp* Op : Ops)
        {
            Op->UpdateCycleCount(VarCount);
            AddCycles(Op->GetCycleCount());
        }
        break;
    case ConLineKind::If:
    case ConLineKind::Loop:
    case ConLineKind::Redo:
    case ConLineKind::Jump:
    {
        int32 BaseCost = 0;
        switch (Kind)
        {
        case ConLineKind::If:
            BaseCost = 1;
            break;
        case ConLineKind::Loop:
            BaseCost = 0;
            break;
        case ConLineKind::Redo:
        case ConLineKind::Jump:
            BaseCost = 1;
            break;
        default:
            BaseCost = 0;
            break;
        }

        int32 ThreadTouches = 0;
        if (Left != nullptr)
        {
            if (const auto* Cached = dynamic_cast<const ConVariableCached*>(Left))
            {
                ++ThreadTouches;
            }
        }
        if (Right != nullptr && Right != Left)
        {
            if (const auto* Cached = dynamic_cast<const ConVariableCached*>(Right))
            {
                ++ThreadTouches;
            }
        }
        if (Kind == ConLineKind::Redo && Counter != nullptr)
        {
            // Counter is always a cached variable, ensure it's counted once
            if (Counter != Left && Counter != Right)
            {
                ++ThreadTouches;
            }
        }
        AddCycles(BaseCost + VarCount * ThreadTouches);
        break;
    }
    default:
        break;
    }
}

void ConLine::SetOps(const vector<ConBaseOp*>& InOps, ConSourceLocation InLocation)
{
    this->Ops = InOps;
    Kind = ConLineKind::Ops;
    Condition = ConConditionOp::None;
    Left = nullptr;
    Right = nullptr;
    Skip = 0;
    LoopExitIndex = -1;
    TargetIndex = -1;
    Counter = nullptr;
    bInfiniteLoop = false;
    Invert = false;
    Location = InLocation;
}

void ConLine::SetIf(ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, int32 SkipCount, bool bInvert, ConSourceLocation InLocation)
{
    Ops.clear();
    Kind = ConLineKind::If;
    Condition = Op;
    Left = Lhs;
    Right = Rhs;
    Skip = SkipCount;
    Invert = bInvert;
    LoopExitIndex = -1;
    TargetIndex = -1;
    Counter = nullptr;
    bInfiniteLoop = false;
    Location = InLocation;
}

void ConLine::SetLoop(ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, bool bInvert, int32 ExitIndex, ConSourceLocation InLocation)
{
    Ops.clear();
    Kind = ConLineKind::Loop;
    Condition = Op;
    Left = Lhs;
    Right = Rhs;
    Invert = bInvert;
    LoopExitIndex = ExitIndex;
    Skip = 0;
    TargetIndex = -1;
    Counter = nullptr;
    bInfiniteLoop = false;
    Location = InLocation;
}

void ConLine::SetRedo(int32 TargetIndex, ConVariableCached* CounterVar, bool bInfinite, ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, bool bInvert, ConSourceLocation InLocation)
{
    Ops.clear();
    Kind = ConLineKind::Redo;
    this->TargetIndex = TargetIndex;
    Counter = CounterVar;
    bInfiniteLoop = bInfinite;
    Condition = Op;
    Left = Lhs;
    Right = Rhs;
    Invert = bInvert;
    Skip = 0;
    LoopExitIndex = -1;
    Location = InLocation;
}

void ConLine::SetJump(int32 TargetIndex, ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, bool bInvert, ConSourceLocation InLocation)
{
    Ops.clear();
    Kind = ConLineKind::Jump;
    this->TargetIndex = TargetIndex;
    Condition = Op;
    Left = Lhs;
    Right = Rhs;
    Invert = bInvert;
    Skip = 0;
    LoopExitIndex = -1;
    Counter = nullptr;
    bInfiniteLoop = false;
    Location = InLocation;
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

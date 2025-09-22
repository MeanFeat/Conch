#include "line.h"

#include "errors.h"

#include <array>

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
        std::array<ConVariableCached*, 3> Owners = {};
        auto TrackOwner = [&](const VariableRef& Ref)
        {
            if (!Ref.TouchesThread())
            {
                return;
            }
            ConVariableCached* Owner = Ref.GetThreadOwner();
            if (Owner == nullptr)
            {
                return;
            }
            for (int32 Index = 0; Index < ThreadTouches; ++Index)
            {
                if (Owners[Index] == Owner)
                {
                    return;
                }
            }
            Owners[ThreadTouches++] = Owner;
        };

        if (Left.IsValid())
        {
            TrackOwner(Left);
        }
        if (Right.IsValid())
        {
            TrackOwner(Right);
        }
        if (Kind == ConLineKind::Redo && Counter.IsValid())
        {
            TrackOwner(Counter);
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
    Left = VariableRef();
    Right = VariableRef();
    Skip = 0;
    LoopExitIndex = -1;
    TargetIndex = -1;
    Counter = VariableRef();
    bInfiniteLoop = false;
    Invert = false;
    Location = InLocation;
}

void ConLine::SetIf(const ConConditionOp Op, VariableRef Lhs, VariableRef Rhs, const int32 SkipCount, const bool bInvert, const ConSourceLocation InLocation)
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
    Counter = VariableRef();
    bInfiniteLoop = false;
    Location = InLocation;
}

void ConLine::SetLoop(const ConConditionOp Op, VariableRef Lhs, VariableRef Rhs, const bool bInvert, const int32 ExitIndex, const ConSourceLocation InLocation)
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
    Counter = VariableRef();
    bInfiniteLoop = false;
    Location = InLocation;
}

void ConLine::SetRedo(const int32 TargetIndex, VariableRef CounterVar, const bool bInfinite, const ConConditionOp Op, VariableRef Lhs, VariableRef Rhs, const bool bInvert, const ConSourceLocation InLocation)
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

void ConLine::SetJump(const int32 TargetIndex, const ConConditionOp Op, VariableRef Lhs, VariableRef Rhs, const bool bInvert, const ConSourceLocation InLocation)
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
    Counter = VariableRef();
    bInfiniteLoop = false;
    Location = InLocation;
}

bool ConLine::EvaluateCondition() const
{
    if (Condition == ConConditionOp::None)
    {
        return Invert ? false : true;
    }

    if (!Left.IsValid() || !Right.IsValid())
    {
        throw ConRuntimeError(Location, "Condition requires two operands");
    }

    bool Result = true;
    switch (Condition)
    {
    case ConConditionOp::GTR:
        Result = Left.Read() > Right.Read();
        break;
    case ConConditionOp::LSR:
        Result = Left.Read() < Right.Read();
        break;
    case ConConditionOp::EQL:
        Result = Left.Read() == Right.Read();
        break;
    default:
        break;
    }
    return Invert ? !Result : Result;
}

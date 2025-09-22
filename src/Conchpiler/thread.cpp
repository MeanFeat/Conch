#include "common.h"
#include "thread.h"
#include <iostream>
#include <ostream>
#include <utility>

void ConThread::Execute()
{
    assert(!Lines.empty());
    size_t i = 0;
    while (i < Lines.size())
    {
        ConLine& Line = Lines[i];
        switch (Line.GetKind())
        {
        case ConLineKind::Ops:
            Line.Execute();
            ++i;
            for (const ConVariable* Var : Variables)
            {
                const ConVariableCached* Cached = dynamic_cast<const ConVariableCached*>(Var);
                cout << Cached->GetVal() << ", (" << Cached->GetCache() << ") ";
            }
            cout << endl;
            break;
        case ConLineKind::If:
            if (!Line.EvaluateCondition())
            {
                i += Line.GetSkipCount() + 1;
                cout << "FALSE" << endl;
            }
            else
            {
                ++i;
                cout << "TRUE" << endl;
            }
            break;
        case ConLineKind::Loop:
            if (Line.HasCondition() && !Line.EvaluateCondition())
            {
                const int32 ExitIndex = Line.GetLoopExitIndex();
                if (ExitIndex >= 0)
                {
                    i = static_cast<size_t>(ExitIndex);
                }
                else
                {
                    ++i;
                }
            }
            else
            {
                ++i;
            }
            break;
        case ConLineKind::Redo:
        {
            bool bLoop = Line.IsInfiniteLoop();
            if (Line.HasCounter())
            {
                ConVariableCached* Counter = Line.GetCounterVar();
                const int32 NewVal = Counter->GetVal() - 1;
                Counter->SetVal(NewVal);
                bLoop = NewVal != 0;
            }
            else if (Line.HasCondition())
            {
                bLoop = Line.EvaluateCondition();
            }

            if (bLoop)
            {
                const int32 TargetIndex = Line.GetTargetIndex();
                if (TargetIndex >= 0)
                {
                    i = static_cast<size_t>(TargetIndex);
                }
                else
                {
                    ++i;
                }
            }
            else
            {
                ++i;
            }
            break;
        }
        case ConLineKind::Jump:
        {
            bool bJump = true;
            if (Line.HasCondition())
            {
                bJump = Line.EvaluateCondition();
            }
            if (bJump)
            {
                const int32 TargetIndex = Line.GetTargetIndex();
                if (TargetIndex >= 0)
                {
                    i = static_cast<size_t>(TargetIndex);
                }
                else
                {
                    ++i;
                }
            }
            else
            {
                ++i;
            }
            break;
        }
        default:
            ++i;
            break;
        }
    }
}

void ConThread::UpdateCycleCount()
{
    ConCompilable::UpdateCycleCount();
    const int32 VarCount = int32(Variables.size());
    for (ConLine& Line : Lines)
    {
        Line.UpdateCycleCount(VarCount);
        AddCycles(Line.GetCycleCount());
    }
}

void ConThread::SetVariables(const vector<ConVariable*>& InVariables)
{
    Variables = InVariables;
}

void ConThread::SetOwnedStorage(std::vector<std::unique_ptr<ConVariableCached>>&& CachedVars,
                                std::vector<std::unique_ptr<ConVariableAbsolute>>&& ConstVars,
                                std::vector<std::unique_ptr<ConVariableList>>&& ListVars,
                                std::vector<std::unique_ptr<ConBaseOp>>&& Ops)
{
    OwnedVarStorage = std::move(CachedVars);
    OwnedConstStorage = std::move(ConstVars);
    OwnedListStorage = std::move(ListVars);
    OwnedOpStorage = std::move(Ops);
}

void ConThread::ConstructLine(const ConLine &Line)
{
    Lines.push_back(Line);
}

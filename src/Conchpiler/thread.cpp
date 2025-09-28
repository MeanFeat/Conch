#include "common.h"
#include "thread.h"
#include "errors.h"
#include <exception>
#include <iostream>
#include <ostream>
#include <utility>

void ConThread::Execute()
{
    ResetRuntimeErrors();
    size_t i = 0;
    while (i < Lines.size())
    {
        ConLine& Line = Lines[i];
        try
        {
            switch (Line.GetKind())
            {
            case ConLineKind::Ops:
                Line.Execute();
                ++i;
                if (bTraceExecution)
                {
                    for (const ConVariableCached* Var : ThreadVariables)
                    {
                        cout << Var->GetVal() << ", (" << Var->GetCache() << ") ";
                    }
                    cout << endl;
                }
                break;
            case ConLineKind::If:
                if (!Line.EvaluateCondition())
                {
                    i += Line.GetSkipCount() + 1;
                    if (bTraceExecution)
                    {
                        cout << "FALSE" << endl;
                    }
                }
                else
                {
                    ++i;
                    if (bTraceExecution)
                    {
                        cout << "TRUE" << endl;
                    }
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
                    ConVariableCached* Counter = Line.GetCounter().GetThread();
                    if (Counter != nullptr)
                    {
                        const int32 NewVal = Counter->GetVal() - 1;
                        Counter->SetVal(NewVal);
                        bLoop = NewVal != 0;
                    }
                    else
                    {
                        bLoop = false;
                    }
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
        catch (const ConRuntimeError& Error)
        {
            ReportRuntimeError(Error);
            break;
        }
        catch (const std::exception& Ex)
        {
            ConRuntimeError Wrapped(Line.GetLocation(), Ex.what());
            ReportRuntimeError(Wrapped);
            break;
        }
    }
}

void ConThread::UpdateCycleCount()
{
    ConCompilable::UpdateCycleCount();
    const int32 VarCount = int32(ThreadVariables.size());
    for (ConLine& Line : Lines)
    {
        Line.UpdateCycleCount(VarCount);
        AddCycles(Line.GetCycleCount());
    }
}

void ConThread::SetVariables(const vector<ConVariableCached*>& InVariables)
{
    ThreadVariables = InVariables;
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

void ConThread::SetTraceEnabled(const bool bEnabled)
{
    bTraceExecution = bEnabled;
}

ConVariableCached* ConThread::GetThreadVar(const size_t Index)
{
    if (Index >= ThreadVariables.size())
    {
        return nullptr;
    }
    return ThreadVariables[Index];
}

const ConVariableCached* ConThread::GetThreadVar(const size_t Index) const
{
    if (Index >= ThreadVariables.size())
    {
        return nullptr;
    }
    return ThreadVariables[Index];
}

int32 ConThread::GetThreadValue(const size_t Index) const
{
    const ConVariableCached* Var = GetThreadVar(Index);
    return Var != nullptr ? Var->GetVal() : 0;
}

int32 ConThread::GetThreadCacheValue(const size_t Index) const
{
    const ConVariableCached* Var = GetThreadVar(Index);
    return Var != nullptr ? Var->GetCache() : 0;
}

void ConThread::SetThreadValue(const size_t Index, const int32 Value)
{
    ConVariableCached* Var = GetThreadVar(Index);
    if (Var != nullptr)
    {
        Var->SetVal(Value);
    }
}

ConVariableList* ConThread::GetListVar(const size_t Index)
{
    if (Index >= OwnedListStorage.size())
    {
        return nullptr;
    }
    return OwnedListStorage[Index].get();
}

const ConVariableList* ConThread::GetListVar(const size_t Index) const
{
    if (Index >= OwnedListStorage.size())
    {
        return nullptr;
    }
    return OwnedListStorage[Index].get();
}

void ConThread::ReportRuntimeError(const ConRuntimeError& Error)
{
    bHadRuntimeError = true;
    const std::string Formatted = FormatErrorMessage(Error.Location, Error.what());
    RuntimeErrors.push_back(Formatted);
    std::cerr << Formatted << std::endl;
}

void ConThread::ResetRuntimeErrors()
{
    RuntimeErrors.clear();
    bHadRuntimeError = false;
}

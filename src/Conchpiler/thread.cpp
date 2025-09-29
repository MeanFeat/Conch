#include "common.h"
#include "thread.h"
#include "errors.h"
#include <exception>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string RegisterName(size_t Index)
{
    switch (Index)
    {
    case 0:
        return "X";
    case 1:
        return "Y";
    case 2:
        return "Z";
    default:
        break;
    }
    std::ostringstream Oss;
    Oss << "R" << Index;
    return Oss.str();
}

struct TraceSnapshot
{
    TraceSnapshot() : BoundThread(nullptr) {}

    void Reset(const vector<ConVariableCached*>& ThreadVariables)
    {
        BoundThread = &ThreadVariables;
        Values.resize(ThreadVariables.size());
        Caches.resize(ThreadVariables.size());
        Defined.resize(ThreadVariables.size());
        for (size_t i = 0; i < ThreadVariables.size(); ++i)
        {
            const ConVariableCached* Var = ThreadVariables[i];
            if (Var != nullptr)
            {
                Defined[i] = true;
                Values[i] = Var->GetVal();
                Caches[i] = Var->GetCache();
            }
            else
            {
                Defined[i] = false;
                Values[i] = 0;
                Caches[i] = 0;
            }
        }
    }

    void EnsureAligned(const vector<ConVariableCached*>& ThreadVariables)
    {
        if (BoundThread != &ThreadVariables || Values.size() != ThreadVariables.size())
        {
            Reset(ThreadVariables);
        }
    }

    const vector<ConVariableCached*>* BoundThread;
    std::vector<int32> Values;
    std::vector<int32> Caches;
    std::vector<bool> Defined;
};

TraceSnapshot& GetTraceSnapshot()
{
    static TraceSnapshot Snapshot;
    return Snapshot;
}

void ResetTraceSnapshot(const vector<ConVariableCached*>& ThreadVariables)
{
    TraceSnapshot& Snapshot = GetTraceSnapshot();
    Snapshot.Reset(ThreadVariables);
}

std::string FormatRegisterState(const vector<ConVariableCached*>& ThreadVariables)
{
    TraceSnapshot& Snapshot = GetTraceSnapshot();
    Snapshot.EnsureAligned(ThreadVariables);

    std::ostringstream Oss;
    bool bAnyChanges = false;
    for (size_t i = 0; i < ThreadVariables.size(); ++i)
    {
        const ConVariableCached* Var = ThreadVariables[i];
        const bool bDefined = Var != nullptr;
        const int32 Value = bDefined ? Var->GetVal() : 0;
        const int32 Cache = bDefined ? Var->GetCache() : 0;

        if (Snapshot.Defined.size() <= i)
        {
            Snapshot.Defined.resize(i + 1, false);
            Snapshot.Values.resize(i + 1, 0);
            Snapshot.Caches.resize(i + 1, 0);
        }

        bool bChanged = Snapshot.Defined[i] != bDefined;
        if (!bChanged && bDefined)
        {
            if (Snapshot.Values[i] != Value || Snapshot.Caches[i] != Cache)
            {
                bChanged = true;
            }
        }

        if (bChanged)
        {
            if (bAnyChanges)
            {
                Oss << "  ";
            }
            Oss << RegisterName(i) << '=';
            if (bDefined)
            {
                Oss << Value << " (C=" << Cache << ')';
            }
            else
            {
                Oss << "<undef>";
            }
            bAnyChanges = true;
        }

        Snapshot.Defined[i] = bDefined;
        if (bDefined)
        {
            Snapshot.Values[i] = Value;
            Snapshot.Caches[i] = Cache;
        }
        else
        {
            Snapshot.Values[i] = 0;
            Snapshot.Caches[i] = 0;
        }
    }

    if (!bAnyChanges)
    {
        return std::string();
    }

    return std::string("| ") + Oss.str();
}

int32 ResolveLineNumber(const ConSourceLocation& Location, size_t Index)
{
    if (Location.IsValid())
    {
        return Location.Line;
    }
    return static_cast<int32>(Index + 1);
}

void PrintTrace(const char* Label,
                const ConSourceLocation& Location,
                size_t Index,
                const std::string& SourceText,
                const vector<ConVariableCached*>& ThreadVariables)
{
    static const char* const TRACE_COLOR = "\033[35m";
    static const char* const REGISTER_COLOR = "\033[36m";
    static const char* const RESET_COLOR = "\033[0m";
    static const size_t REGISTER_COLUMN = 64;

    std::ostringstream PrefixStream;
    PrefixStream << "[Line " << ResolveLineNumber(Location, Index);
    if (Label != nullptr && Label[0] != '\0')
    {
        PrefixStream << ' ' << Label;
    }
    if (!SourceText.empty())
    {
        PrefixStream << "] " << SourceText;
    }
    else
    {
        PrefixStream << "]";
    }

    const std::string Prefix = PrefixStream.str();
    const size_t VisiblePrefixLength = Prefix.size();
    const std::string RegisterState = FormatRegisterState(ThreadVariables);

    std::ostringstream Oss;
    Oss << TRACE_COLOR << Prefix;
    if (!RegisterState.empty())
    {
        size_t Padding = 1;
        if (VisiblePrefixLength < REGISTER_COLUMN)
        {
            Padding = REGISTER_COLUMN - VisiblePrefixLength;
        }
        Oss << std::string(Padding, ' ') << REGISTER_COLOR << RegisterState << RESET_COLOR;
    }
    else
    {
        Oss << RESET_COLOR;
    }

    std::cout << Oss.str() << std::endl;
}
}

void ConThread::Execute()
{
    ResetRuntimeErrors();
    ResetTraceSnapshot(ThreadVariables);
    size_t i = 0;
    std::vector<int32> LoopIterations(Lines.size(), 0);
    constexpr int32 LoopIterationLimit = 9999;
    while (i < Lines.size())
    {
        ConLine& Line = Lines[i];
        const ConSourceLocation Location = Line.GetLocation();
        const size_t LineIndex = i;
        try
        {
            switch (Line.GetKind())
            {
            case ConLineKind::Ops:
            {
                Line.Execute();
                ++i;
                if (bTraceExecution)
                {
                    PrintTrace("OPS", Location, LineIndex, Line.GetSourceText(), ThreadVariables);
                }
                break;
            }
            case ConLineKind::If:
            {
                const bool bCondition = Line.EvaluateCondition();
                if (!bCondition)
                {
                    i += Line.GetSkipCount() + 1;
                }
                else
                {
                    ++i;
                }
                if (bTraceExecution)
                {
                    PrintTrace(bCondition ? "IF=TRUE" : "IF=FALSE", Location, LineIndex, Line.GetSourceText(), ThreadVariables);
                }
                break;
            }
            case ConLineKind::Loop:
            {
                const int32 ExitIndex = Line.GetLoopExitIndex();
                const int32 RedoIndex = ExitIndex > 0 ? ExitIndex - 1 : -1;
                bool bRuns = true;
                if (Line.HasCondition())
                {
                    const bool bCondition = Line.EvaluateCondition();
                    if (!bCondition)
                    {
                        bRuns = false;
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
                }
                else
                {
                    ++i;
                }
                if (!bRuns && RedoIndex >= 0)
                {
                    const size_t RedoIdx = static_cast<size_t>(RedoIndex);
                    if (RedoIdx < LoopIterations.size())
                    {
                        LoopIterations[RedoIdx] = 0;
                    }
                }
                if (bTraceExecution)
                {
                    PrintTrace(bRuns ? "REDO-HEAD" : "REDO-SKIP", Location, LineIndex, Line.GetSourceText(), ThreadVariables);
                }
                break;
            }
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
                    int32& IterationCount = LoopIterations[LineIndex];
                    ++IterationCount;
                    if (IterationCount > LoopIterationLimit)
                    {
                        throw ConRuntimeError(Location, "Loop exceeded 9999 iterations");
                    }

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
                    LoopIterations[LineIndex] = 0;
                    ++i;
                }
                if (bTraceExecution)
                {
                    PrintTrace(bLoop ? "REDO" : "REDO-EXIT", Location, LineIndex, Line.GetSourceText(), ThreadVariables);
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
                if (bTraceExecution)
                {
                    PrintTrace(bJump ? "JUMP" : "NO-JUMP", Location, LineIndex, Line.GetSourceText(), ThreadVariables);
                }
                break;
            }
            default:
            {
                ++i;
                if (bTraceExecution)
                {
                    PrintTrace("STEP", Location, LineIndex, Line.GetSourceText(), ThreadVariables);
                }
                break;
            }
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

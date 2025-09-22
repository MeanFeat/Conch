#include "op.h"

#include <array>
#include <utility>

ConBaseOp::ConBaseOp(const vector<VariableRef>& InArgs)
    : Args(InArgs)
{
    RefreshThreadMixSummary();
}

void ConBaseOp::SetArgs(vector<VariableRef> InArgs)
{
    if (InArgs.size() > static_cast<size_t>(GetMaxArgs()))
    {
        throw ConRuntimeError(GetSourceLocation(), "Too many arguments supplied to operation");
    }
    Args = std::move(InArgs);
    RefreshThreadMixSummary();
}

VariableRef ConBaseOp::GetReturn() const
{
    if (!HasReturn() || GetArgs().empty())
    {
        return VariableRef();
    }
    return GetArgs().front();
}

const vector<VariableRef>& ConBaseOp::GetArgs() const
{
    return Args;
}

vector<VariableRef>& ConBaseOp::GetMutableArgs()
{
    return Args;
}

VariableRef& ConBaseOp::GetArgRef(const int32 Index)
{
    if (Index < 0 || static_cast<size_t>(Index) >= Args.size())
    {
        throw ConRuntimeError(GetSourceLocation(), "Argument index out of range");
    }
    return Args.at(static_cast<size_t>(Index));
}

const VariableRef& ConBaseOp::GetArgRef(const int32 Index) const
{
    if (Index < 0 || static_cast<size_t>(Index) >= Args.size())
    {
        throw ConRuntimeError(GetSourceLocation(), "Argument index out of range");
    }
    return Args.at(static_cast<size_t>(Index));
}

void ConBaseOp::RefreshThreadMixSummary()
{
    MixSummary = {};
    std::array<ConVariableCached*, 4> Owners = {};
    int32 OwnerCount = 0;

    for (const VariableRef& Ref : Args)
    {
        if (!Ref.IsValid())
        {
            continue;
        }
        if (Ref.IsLiteral())
        {
            MixSummary.UsesLiteral = true;
        }
        if (Ref.TouchesThread())
        {
            if (Ref.IsCache())
            {
                MixSummary.HasCacheReads = true;
            }
            ConVariableCached* Owner = Ref.GetThreadOwner();
            if (Owner == nullptr)
            {
                continue;
            }
            bool bExisting = false;
            for (int32 Index = 0; Index < OwnerCount; ++Index)
            {
                if (Owners[Index] == Owner)
                {
                    bExisting = true;
                    break;
                }
            }
            if (!bExisting && OwnerCount < static_cast<int32>(Owners.size()))
            {
                Owners[OwnerCount++] = Owner;
            }
        }
    }

    MixSummary.UniqueThreadVars = OwnerCount;
}

void ConBaseOp::UpdateCycleCount()
{
    UpdateCycleCount(0);
}

void ConBaseOp::UpdateCycleCount(const int32 VarCount)
{
    ConCompilable::UpdateCycleCount();
    const int32 VariableAccesses = GetVariableAccessCount();
    AddCycles(GetBaseCycleCost() + VarCount * VariableAccesses);
}

int32 ConBaseOp::GetVariableAccessCount() const
{
    return MixSummary.UniqueThreadVars;
}

bool ConBaseOp::MixesMultipleThreadVariables() const
{
    return MixSummary.UniqueThreadVars > 1;
}

std::vector<ConVariableCached*> ConBaseOp::GetThreadParticipants() const
{
    std::vector<ConVariableCached*> Participants;
    Participants.reserve(static_cast<size_t>(MixSummary.UniqueThreadVars));
    for (const VariableRef& Ref : Args)
    {
        if (!Ref.TouchesThread())
        {
            continue;
        }
        ConVariableCached* Owner = Ref.GetThreadOwner();
        if (Owner == nullptr)
        {
            continue;
        }
        bool bExists = false;
        for (ConVariableCached* Existing : Participants)
        {
            if (Existing == Owner)
            {
                bExists = true;
                break;
            }
        }
        if (!bExists)
        {
            Participants.push_back(Owner);
        }
    }
    return Participants;
}

VariableRef& ConContextualReturnOp::GetDstArg()
{
    if (GetArgsCount() == 0)
    {
        throw ConRuntimeError(GetSourceLocation(), "Operation missing destination argument");
    }
    return GetMutableArgs().at(0);
}

const VariableRef& ConContextualReturnOp::GetDstArg() const
{
    if (GetArgsCount() == 0)
    {
        throw ConRuntimeError(GetSourceLocation(), "Operation missing destination argument");
    }
    return GetArgs().at(0);
}

vector<VariableRef> ConContextualReturnOp::GetSrcArg() const
{
    if (GetArgsCount() < 2)
    {
        throw ConRuntimeError(GetSourceLocation(), "Operation missing source argument");
    }
    if (GetArgsCount() == 3)
    {
        return {GetArgs().at(1), GetArgs().at(2)};
    }
    return {GetDstArg(), GetArgs().at(1)};
}

ConBinaryOp::ConBinaryOp(const ConBinaryOpKind InKind, const vector<VariableRef>& InArgs)
    : ConContextualReturnOp(InArgs)
    , Kind(InKind)
{
    const vector<VariableRef> SrcArgs = GetSrcArg();
    if (SrcArgs.size() == 2 && SrcArgs[0].IsLiteral() && SrcArgs[1].IsLiteral())
    {
        bHasPrecomputed = true;
        PrecomputedValue = Compute(SrcArgs[0].Read(), SrcArgs[1].Read());
    }
}

int32 ConBinaryOp::Compute(const int32 Lhs, const int32 Rhs) const
{
    switch (Kind)
    {
    case ConBinaryOpKind::Add:
        return Lhs + Rhs;
    case ConBinaryOpKind::Sub:
        return Lhs - Rhs;
    case ConBinaryOpKind::Mul:
        return Lhs * Rhs;
    case ConBinaryOpKind::Div:
        return (Rhs == 0) ? 0 : Lhs / Rhs;
    case ConBinaryOpKind::And:
        return Lhs & Rhs;
    case ConBinaryOpKind::Or:
        return Lhs | Rhs;
    case ConBinaryOpKind::Xor:
        return Lhs ^ Rhs;
    default:
        throw ConRuntimeError(GetSourceLocation(), "Unknown binary operation");
    }
}

void ConBinaryOp::Execute()
{
    const vector<VariableRef> SrcArg = GetSrcArg();
    if (SrcArg.size() < 2 || !SrcArg.at(0).IsValid() || !SrcArg.at(1).IsValid())
    {
        throw ConRuntimeError(GetSourceLocation(), "Binary operation missing operands");
    }

    VariableRef& DstRef = GetDstArg();
    if (!DstRef.IsThread())
    {
        throw ConRuntimeError(GetSourceLocation(), "Binary operation destination must be a thread variable");
    }

    ConVariableCached* Dst = DstRef.GetThread();
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "Binary operation destination is invalid");
    }

    const int32 Result = bHasPrecomputed ? PrecomputedValue : Compute(SrcArg.at(0).Read(), SrcArg.at(1).Read());
    Dst->SetVal(Result);
}

void ConIncrOp::Execute()
{
    if (GetArgsCount() < 1)
    {
        throw ConRuntimeError(GetSourceLocation(), "INCR requires a destination argument");
    }
    VariableRef& DstRef = GetArgRef(0);
    if (!DstRef.IsThread())
    {
        throw ConRuntimeError(GetSourceLocation(), "INCR destination must be a thread variable");
    }
    ConVariableCached* Dst = DstRef.GetThread();
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "INCR destination is invalid");
    }
    Dst->SetVal(Dst->GetVal() + 1);
}

void ConDecrOp::Execute()
{
    if (GetArgsCount() < 1)
    {
        throw ConRuntimeError(GetSourceLocation(), "DECR requires a destination argument");
    }
    VariableRef& DstRef = GetArgRef(0);
    if (!DstRef.IsThread())
    {
        throw ConRuntimeError(GetSourceLocation(), "DECR destination must be a thread variable");
    }
    ConVariableCached* Dst = DstRef.GetThread();
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "DECR destination is invalid");
    }
    Dst->SetVal(Dst->GetVal() - 1);
}

void ConNotOp::Execute()
{
    if (GetArgsCount() < 1)
    {
        throw ConRuntimeError(GetSourceLocation(), "NOT requires a destination argument");
    }
    VariableRef& DstRef = GetArgRef(0);
    if (!DstRef.IsThread())
    {
        throw ConRuntimeError(GetSourceLocation(), "NOT destination must be a thread variable");
    }
    ConVariableCached* Dst = DstRef.GetThread();
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "NOT destination is invalid");
    }
    if (GetArgsCount() == 1)
    {
        Dst->SetVal(~Dst->GetVal());
    }
    else
    {
        const VariableRef& Src = GetArgRef(1);
        if (!Src.IsValid())
        {
            throw ConRuntimeError(GetSourceLocation(), "NOT source argument is invalid");
        }
        Dst->SetVal(~Src.Read());
    }
}

void ConPopOp::Execute()
{
    if (GetArgsCount() < 2)
    {
        throw ConRuntimeError(GetSourceLocation(), "POP requires a destination and a list argument");
    }
    VariableRef& DstRef = GetArgRef(0);
    if (!DstRef.IsThread())
    {
        throw ConRuntimeError(GetSourceLocation(), "POP destination must be a thread variable");
    }
    ConVariableCached* Dst = DstRef.GetThread();
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "POP destination is invalid");
    }
    const VariableRef& ListRef = GetArgRef(1);
    ConVariableList* List = ListRef.GetList();
    if (List == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "POP requires a list operand");
    }
    Dst->SetVal(List->Pop());
}

void ConAtOp::Execute()
{
    if (GetArgsCount() < 3)
    {
        throw ConRuntimeError(GetSourceLocation(), "AT requires a destination, list, and index");
    }
    VariableRef& DstRef = GetArgRef(0);
    if (!DstRef.IsThread())
    {
        throw ConRuntimeError(GetSourceLocation(), "AT destination must be a thread variable");
    }
    ConVariableCached* Dst = DstRef.GetThread();
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "AT destination is invalid");
    }
    const VariableRef& ListRef = GetArgRef(1);
    ConVariableList* List = ListRef.GetList();
    if (List == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "AT requires a list operand");
    }
    const VariableRef& IndexRef = GetArgRef(2);
    if (!IndexRef.IsValid())
    {
        throw ConRuntimeError(GetSourceLocation(), "AT index argument is invalid");
    }
    Dst->SetVal(List->At(IndexRef.Read()));
}

void ConSetOp::Execute()
{
    if (GetArgsCount() < 2)
    {
        throw ConRuntimeError(GetSourceLocation(), "SET requires a destination and a source");
    }
    VariableRef& DstRef = GetArgRef(0);
    if (!DstRef.IsThread())
    {
        throw ConRuntimeError(GetSourceLocation(), "SET destination must be a thread variable");
    }
    ConVariableCached* Dst = DstRef.GetThread();
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "SET destination is invalid");
    }
    const VariableRef& SrcRef = GetArgRef(1);
    if (!SrcRef.IsValid())
    {
        throw ConRuntimeError(GetSourceLocation(), "SET source argument is invalid");
    }
    Dst->SetVal(SrcRef.Read());
}

void ConSwpOp::Execute()
{
    if (GetArgsCount() < 1)
    {
        throw ConRuntimeError(GetSourceLocation(), "SWP requires a destination argument");
    }
    VariableRef& DstRef = GetArgRef(0);
    if (!DstRef.IsThread())
    {
        throw ConRuntimeError(GetSourceLocation(), "SWP destination must be a thread variable");
    }
    ConVariableCached* Dst = DstRef.GetThread();
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "SWP destination is invalid");
    }
    Dst->Swap();
}

#include "op.h"

#include <unordered_set>

void ConBaseOp::SetArgs(const vector<ConVariable*> Args)
{
    if (Args.size() > static_cast<size_t>(GetMaxArgs()))
    {
        throw ConRuntimeError(GetSourceLocation(), "Too many arguments supplied to operation");
    }
    this->Args = Args;
}

ConVariable* ConBaseOp::GetReturn() const
{
    if (!HasReturn() || GetArgs().empty())
    {
        return nullptr;
    }
    return GetArgs().front();
}

const vector<ConVariable*> &ConBaseOp::GetArgs() const
{
    return Args;
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
    std::unordered_set<const ConVariableCached*> UniqueThreadVars;
    for (const ConVariable* Var : GetArgs())
    {
        if (const auto* Cached = dynamic_cast<const ConVariableCached*>(Var))
        {
            UniqueThreadVars.insert(Cached);
        }
    }
    return static_cast<int32>(UniqueThreadVars.size());
}


ConVariable* ConContextualReturnOp::GetDstArg() const
{
    if (GetArgsCount() == 0)
    {
        throw ConRuntimeError(GetSourceLocation(), "Operation missing destination argument");
    }
    return GetArgs().at(0);
}

vector<const ConVariable*> ConContextualReturnOp::GetSrcArg() const
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

ConBinaryOp::ConBinaryOp(const ConBinaryOpKind InKind, const vector<ConVariable*>& InArgs)
    : ConContextualReturnOp(InArgs)
    , Kind(InKind)
{
}

void ConBinaryOp::Execute()
{
    const vector<const ConVariable*> SrcArg = GetSrcArg();
    if (SrcArg.size() < 2 || SrcArg.at(0) == nullptr || SrcArg.at(1) == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "Binary operation missing operands");
    }

    ConVariableCached* Dst = dynamic_cast<ConVariableCached*>(GetDstArg());
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "Binary operation destination must be a thread variable");
    }

    const int32 Lhs = SrcArg.at(0)->GetVal();
    const int32 Rhs = SrcArg.at(1)->GetVal();

    int32 Result = 0;
    switch (Kind)
    {
    case ConBinaryOpKind::Add:
        Result = Lhs + Rhs;
        break;
    case ConBinaryOpKind::Sub:
        Result = Lhs - Rhs;
        break;
    case ConBinaryOpKind::Mul:
        Result = Lhs * Rhs;
        break;
    case ConBinaryOpKind::Div:
        Result = (Rhs == 0) ? 0 : Lhs / Rhs;
        break;
    case ConBinaryOpKind::And:
        Result = Lhs & Rhs;
        break;
    case ConBinaryOpKind::Or:
        Result = Lhs | Rhs;
        break;
    case ConBinaryOpKind::Xor:
        Result = Lhs ^ Rhs;
        break;
    default:
        throw ConRuntimeError(GetSourceLocation(), "Unknown binary operation");
    }

    Dst->SetVal(Result);
}

void ConIncrOp::Execute()
{
    if (GetArgsCount() < 1)
    {
        throw ConRuntimeError(GetSourceLocation(), "INCR requires a destination argument");
    }
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "INCR destination must be a thread variable");
    }
    Dst->SetVal(Dst->GetVal() + 1);
}

void ConDecrOp::Execute()
{
    if (GetArgsCount() < 1)
    {
        throw ConRuntimeError(GetSourceLocation(), "DECR requires a destination argument");
    }
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "DECR destination must be a thread variable");
    }
    Dst->SetVal(Dst->GetVal() - 1);
}

void ConNotOp::Execute()
{
    if (GetArgsCount() < 1)
    {
        throw ConRuntimeError(GetSourceLocation(), "NOT requires a destination argument");
    }
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "NOT destination must be a thread variable");
    }
    if (GetArgsCount() == 1)
    {
        Dst->SetVal(~Dst->GetVal());
    }
    else
    {
        const ConVariable* Src = GetArgAs<ConVariable*>(1);
        if (Src == nullptr)
        {
            throw ConRuntimeError(GetSourceLocation(), "NOT source argument is invalid");
        }
        Dst->SetVal(~Src->GetVal());
    }
}

void ConPopOp::Execute()
{
    if (GetArgsCount() < 2)
    {
        throw ConRuntimeError(GetSourceLocation(), "POP requires a destination and a list argument");
    }
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "POP destination must be a thread variable");
    }
    ConVariableList* List = dynamic_cast<ConVariableList*>(GetArgAs<ConVariable*>(1));
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
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "AT destination must be a thread variable");
    }
    ConVariableList* List = dynamic_cast<ConVariableList*>(GetArgAs<ConVariable*>(1));
    if (List == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "AT requires a list operand");
    }
    const ConVariable* IndexVar = GetArgAs<ConVariable*>(2);
    if (IndexVar == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "AT index argument is invalid");
    }
    Dst->SetVal(List->At(IndexVar->GetVal()));
}

void ConSetOp::Execute()
{
    if (GetArgsCount() < 2)
    {
        throw ConRuntimeError(GetSourceLocation(), "SET requires a destination and a source");
    }
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "SET destination must be a thread variable");
    }
    const ConVariable* Src = GetArgAs<ConVariable*>(1);
    if (Src == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "SET source argument is invalid");
    }
    Dst->SetVal(Src->GetVal());
}

void ConSwpOp::Execute()
{
    if (GetArgsCount() < 1)
    {
        throw ConRuntimeError(GetSourceLocation(), "SWP requires a destination argument");
    }
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    if (Dst == nullptr)
    {
        throw ConRuntimeError(GetSourceLocation(), "SWP destination must be a thread variable");
    }
    Dst->Swap();
}




#include "op.h"
#include <unordered_set>

void ConBaseOp::SetArgs(const vector<ConVariable*> Args)
{
    assert(GetArgsCount() <= GetMaxArgs());
    this->Args = Args;
}

ConVariable* ConBaseOp::GetReturn() const
{
    assert(HasReturn());
    return nullptr;
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
    assert(GetArgsCount() > 0);
    return GetArgs().at(0);
}

vector<const ConVariable*> ConContextualReturnOp::GetSrcArg() const
{
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
        assert(false);
        break;
    }

    GetDstArg()->SetVal(Result);
}

void ConIncrOp::Execute()
{
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    Dst->SetVal(Dst->GetVal() + 1);
}

void ConDecrOp::Execute()
{
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    Dst->SetVal(Dst->GetVal() - 1);
}

void ConNotOp::Execute()
{
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    if (GetArgsCount() == 1)
    {
        Dst->SetVal(~Dst->GetVal());
    }
    else
    {
        const ConVariable* Src = GetArgAs<ConVariable*>(1);
        Dst->SetVal(~Src->GetVal());
    }
}

void ConPopOp::Execute()
{
    assert(GetArgsCount() >= 2);
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    ConVariableList* List = dynamic_cast<ConVariableList*>(GetArgAs<ConVariable*>(1));
    assert(List != nullptr);
    Dst->SetVal(List->Pop());
}

void ConAtOp::Execute()
{
    assert(GetArgsCount() >= 3);
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    ConVariableList* List = dynamic_cast<ConVariableList*>(GetArgAs<ConVariable*>(1));
    assert(List != nullptr);
    const ConVariable* IndexVar = GetArgAs<ConVariable*>(2);
    Dst->SetVal(List->At(IndexVar->GetVal()));
}

void ConSetOp::Execute()
{
    assert(dynamic_cast<ConVariableCached*>(GetArgs().at(0)) != nullptr);
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    const ConVariable* Src = GetArgAs<ConVariable*>(1);
    Dst->SetVal(Src->GetVal());
}

void ConSwpOp::Execute()
{
    assert(dynamic_cast<ConVariableCached*>(GetArgs().at(0)) != nullptr);
    ConVariableCached* Dst = GetArgAs<ConVariableCached*>(0);
    Dst->Swap();
}




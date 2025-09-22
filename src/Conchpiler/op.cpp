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

void ConAddOp::Execute()
{
    const vector<const ConVariable*> SrcArg = GetSrcArg();
    GetDstArg()->SetVal(SrcArg.at(0)->GetVal() + SrcArg.at(1)->GetVal());   
}

void ConMulOp::Execute()
{
    const vector<const ConVariable*> SrcArg = GetSrcArg();
    GetDstArg()->SetVal(SrcArg.at(0)->GetVal() * SrcArg.at(1)->GetVal()); 
}

void ConSubOp::Execute()
{
    const vector<const ConVariable*> SrcArg = GetSrcArg();
    GetDstArg()->SetVal(SrcArg.at(0)->GetVal() - SrcArg.at(1)->GetVal());
}

void ConDivOp::Execute()
{
    const vector<const ConVariable*> SrcArg = GetSrcArg();
    const int32 B = SrcArg.at(1)->GetVal();
    GetDstArg()->SetVal(B == 0 ? 0 : SrcArg.at(0)->GetVal() / B);
}

void ConSetOp::Execute()
{
    ConVariableCached *Dst = GetArgAs<ConVariableCached*>(0);
    const ConVariableCached *Src = GetArgAs<ConVariableCached*>(1);
    Dst->SetVal(Src->GetVal());
}

void ConSwpOp::Execute()
{
    ConVariableCached *Dst = GetArgAs<ConVariableCached*>(0);
    Dst->Swap();   
}




#include "op.h"

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

void ConAddOp::Execute()
{
    if (GetArgsCount() == 2)
    {
        // inplace
        ConVariableCached *Dst = GetArgAs<ConVariableCached*>(0);
        const ConVariable *Src = GetArgs().at(1);
        Dst->SetVal(Dst->GetVal() + Src->GetVal());
    }
    else if (GetArgsCount() == 3)
    {
        ConVariableCached *Dst = GetArgAs<ConVariableCached*>(0);
        Dst->SetVal(GetArgAs<ConVariableCached*>(1)->GetVal() + GetArgAs<ConVariableCached*>(2)->GetVal());
    }
}


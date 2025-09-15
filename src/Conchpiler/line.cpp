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
    ConCompilable::UpdateCycleCount();
    for (ConBaseOp* Op : Ops)
    {
        Op->UpdateCycleCount();
        AddCycles(Op->GetCycleCount());
    }
}

void ConLine::SetOps(const vector<ConBaseOp*>& Ops)
{
    this->Ops = Ops;   
}

#include "line.h"

ConLine::~ConLine()
{
}

void ConLine::Execute(vector<ConVariable*>& regs)
{
    for (ConBaseOp* Op : Ops)
    {
        Op->Execute(regs);
    }
}

void ConLine::UpdateCycleCount()
{
    ConCompilable::UpdateCycleCount();
    for (ConBaseOp* Op : Ops)
    {
        Op->UpdateCycleCount();
    }   
}

void ConLine::SetOps(const vector<ConBaseOp*>& Ops)
{
    this->Ops = Ops;   
}

#include "common.h"
#include "thread.h"

void ConThread::Execute()
{
    assert(Lines.size() > 0);
    for (ConLine& Line : Lines)
    {
        Line.Execute();
    }
}

void ConThread::UpdateCycleCount()
{
    ConCompilable::UpdateCycleCount();
    for (ConLine& Line : Lines)
    {
        Line.UpdateCycleCount();
        AddCycles(Line.GetCycleCount());
    }
}

void ConThread::SetVariables(const vector<ConVariable*>& InVariables)
{
    Variables = InVariables;
}

void ConThread::ConstructLine(const vector<ConBaseOp*> &Ops)
{
    ConLine Line;
    Line.SetOps(Ops);
    Lines.push_back(Line);
}

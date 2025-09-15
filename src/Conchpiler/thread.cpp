#include "common.h"
#include "thread.h"
#include <iostream>
#include <ostream>

void ConThread::Execute()
{
    assert(!Lines.empty());
    size_t i = 0;
    while (i < Lines.size())
    {
        ConLine& Line = Lines[i];
        if (Line.IsConditional())
        {
            if (!Line.EvaluateCondition())
            {
                i += Line.GetSkipCount() + 1;
            }
            else
            {
                ++i;
            }
        }
        else
        {
            Line.Execute();
            ++i;
        }
        for (const ConVariable* Var : Variables)
        {
            const ConVariableCached* Cached = dynamic_cast<const ConVariableCached*>(Var);
            cout << Cached->GetVal() << ", (" << Cached->GetCache() << ") ";
        }
        cout << endl;
    }
}

void ConThread::UpdateCycleCount()
{
    ConCompilable::UpdateCycleCount();
    const int32 VarCount = int32(Variables.size());
    for (ConLine& Line : Lines)
    {
        Line.UpdateCycleCount();
        Line.AddCycles(VarCount);
        AddCycles(Line.GetCycleCount());
    }
}

void ConThread::SetVariables(const vector<ConVariable*>& InVariables)
{
    Variables = InVariables;
}

void ConThread::ConstructLine(const ConLine &Line)
{
    Lines.push_back(Line);
}

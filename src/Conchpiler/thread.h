#pragma once
#include "line.h"
#include "variable.h"

struct ConThread final : public ConCompilable
{
public:
    ConThread(): Variables()
    {
    } ;

    virtual void Execute() override;
    virtual void UpdateCycleCount() override;

    void SetVariables(const vector<ConVariable*>& InVariables);
    void ConstructLine(const vector<ConBaseOp*>& Ops);
    
private:
    vector<ConVariable*> Variables;
    vector<ConLine> Lines;
};

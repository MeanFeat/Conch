#pragma once
#include "common.h"
#include "compilable.h"
#include "op.h"

struct ConLine : public ConCompilable
{
public:

    virtual  ~ConLine() override;
    virtual void Execute() override;
    virtual void UpdateCycleCount() override;
    void SetOps(const vector<ConBaseOp*>& Ops);

private:
    // in reverse order of operation
    vector<ConBaseOp*> Ops;
};


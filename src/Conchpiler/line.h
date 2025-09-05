#pragma once
#include "common.h"
#include "compilable.h"
#include "op.h"

class ConLine : public ConCompilable
{
public:

    virtual  ~ConLine() override;
    
    virtual void Execute() override;

private:
    // in reverse order of operation
    vector<ConBasOp> Ops;
};


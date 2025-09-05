#pragma once
#include "line.h"
#include "variable.h"

struct ConThread : public ConCompilable
{
public:
    virtual ~ConThread() override;
    
    virtual void Execute() override;


private:
    vector<ConVariable> Variables;
    vector<ConLine> Lines;
};

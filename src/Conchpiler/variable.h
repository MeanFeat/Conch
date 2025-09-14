#pragma once
#include "common.h"

struct ConVariable
{
    ConVariable() = default;
    virtual ~ConVariable() = default;
    virtual int32 GetVal() const = 0;
    virtual void SetVal(int32 NewVal) = 0;
};

struct ConVariableCached : public ConVariable
{
    virtual int32 GetVal() const override;
    virtual void SetVal(int32 NewVal) override;
    int32 GetCache() const;
    // swaps the value and the cache
    void Swap();
    ConVariableCached SwapInPlace();
    
private:
    
    int32 Val = 0;
    int32 Cache = 0;
};

struct ConVariableList : public vector<ConVariable>
{
    
};
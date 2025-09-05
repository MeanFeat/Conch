#pragma once
#include "common.h"

struct ConVariable
{
    ConVariable() = default;
    virtual ~ConVariable() = default;
    virtual int32 GetVal() const = 0;
};

struct ConVariableAbsolute final : public ConVariable
{
    ConVariableAbsolute() = default;
    ConVariableAbsolute(const int32 InVal)
        : Val(InVal)
    {
    }
    virtual int32 GetVal() const override;
    void SetVal(int32 NewVal);
private:
    int32 Val = 0;
};

struct ConVariableCached final : public ConVariable
{
    virtual int32 GetVal() const override;
    void SetVal(int32 NewVal);
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
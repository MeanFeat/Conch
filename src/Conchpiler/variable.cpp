#include "variable.h"

int32 ConVariableAbsolute::GetVal() const
{
    return Val;
}

void ConVariableAbsolute::SetVal(const int32 NewVal)
{
    Val = NewVal;   
}

int32 ConVariableCached::GetVal() const
{
    return Val;
}

void ConVariableCached::SetVal(const int32 NewVal)
{
    Cache = Val;
    Val = NewVal;
}

int32 ConVariableCached::GetCache() const
{
    return Cache;
}

void ConVariableCached::Swap()
{
    const int32 Temp = Val;
    Val = Cache;
    Cache = Temp;
}

ConVariableCached ConVariableCached::SwapInPlace()
{
    const int32 Temp = Val;
    Val = Cache;
    Cache = Temp;
    return *this;
}

#include "variable.h"

int32 ConVariable::GetVal() const
{
    return Val;
}

void ConVariable::SetVal(const int32 NewVal)
{
    Cache = Val;
    Val = NewVal;
}

int32 ConVariable::GetCache() const
{
    return Cache;
}

void ConVariable::Swap()
{
    const int32 Temp = Val;
    Val = Cache;
    Cache = Temp;
}

ConVariable ConVariable::SwapInPlace()
{
    const int32 Temp = Val;
    Val = Cache;
    Cache = Temp;
    return *this;
}

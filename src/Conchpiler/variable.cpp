#include "variable.h"

ConVariableCached::ConVariableCached()
    : CacheAccessor(*this)
{
}

ConVariableCached::ConVariableCached(const ConVariableCached& Other)
    : CacheAccessor(*this)
    , Val(Other.Val)
    , Cache(Other.Cache)
{
}

ConVariableCached& ConVariableCached::operator=(const ConVariableCached& Other)
{
    if (this != &Other)
    {
        Val = Other.Val;
        Cache = Other.Cache;
    }
    return *this;
}

ConVariableCached::ConVariableCached(ConVariableCached&& Other) noexcept
    : CacheAccessor(*this)
    , Val(Other.Val)
    , Cache(Other.Cache)
{
}

ConVariableCached& ConVariableCached::operator=(ConVariableCached&& Other) noexcept
{
    if (this != &Other)
    {
        Val = Other.Val;
        Cache = Other.Cache;
    }
    return *this;
}

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

void ConVariableCached::SetCache(const int32 NewVal)
{
    Cache = NewVal;
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

ConVariable* ConVariableCached::GetCacheVariable()
{
    return &CacheAccessor;
}

const ConVariable* ConVariableCached::GetCacheVariable() const
{
    return &CacheAccessor;
}

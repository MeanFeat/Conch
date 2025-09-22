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

ConVariableList::ConVariableList(const vector<int32>& InValues)
    : Storage(InValues)
{
    if (!Storage.empty())
    {
        CurrentValue = Storage.front();
    }
}

int32 ConVariableList::GetVal() const
{
    return CurrentValue;
}

void ConVariableList::SetVal(const int32 NewVal)
{
    Push(NewVal);
}

int32 ConVariableList::Pop()
{
    if (Cursor < Storage.size())
    {
        CurrentValue = Storage.at(Cursor);
        ++Cursor;
    }
    else
    {
        CurrentValue = 0;
    }
    return CurrentValue;
}

int32 ConVariableList::At(const int32 Index) const
{
    if (Index >= 0 && static_cast<size_t>(Index) < Storage.size())
    {
        CurrentValue = Storage.at(static_cast<size_t>(Index));
        return CurrentValue;
    }
    CurrentValue = 0;
    return CurrentValue;
}

void ConVariableList::Push(const int32 Value)
{
    Storage.push_back(Value);
    CurrentValue = Value;
}

void ConVariableList::SetValues(const vector<int32>& Values)
{
    Storage = Values;
    Cursor = 0;
    CurrentValue = Storage.empty() ? 0 : Storage.front();
}

bool ConVariableList::Empty() const
{
    return Storage.empty();
}

void ConVariableList::Reset()
{
    Cursor = 0;
    if (!Storage.empty())
    {
        CurrentValue = Storage.front();
    }
    else
    {
        CurrentValue = 0;
    }
}

#pragma once
#include "common.h"

struct ConVariable
{
    ConVariable() = default;
    virtual ~ConVariable() = default;
    virtual int32 GetVal() const = 0;
    virtual void SetVal(int32 NewVal) = 0;
};

struct ConVariableAbsolute : public ConVariable
{
    ConVariableAbsolute() = default;
    ConVariableAbsolute(const int32 InVal)
        : Val(InVal)  {}
    virtual int32 GetVal() const override;
    virtual void SetVal(int32 NewVal) override;
private:
    int32 Val = 0;
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

struct ConVariableList : public ConVariable
{
    ConVariableList() = default;
    explicit ConVariableList(const vector<int32>& InValues);

    virtual int32 GetVal() const override;
    virtual void SetVal(int32 NewVal) override;

    int32 Pop();
    int32 At(int32 Index) const;
    void Push(int32 Value);
    void SetValues(const vector<int32>& Values);
    bool Empty() const;
    void Reset();

private:
    vector<int32> Storage;
    mutable int32 CurrentValue = 0;
    size_t Cursor = 0;
};
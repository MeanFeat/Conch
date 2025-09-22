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
    ConVariableCached();
    ConVariableCached(const ConVariableCached& Other);
    ConVariableCached& operator=(const ConVariableCached& Other);
    ConVariableCached(ConVariableCached&& Other) noexcept;
    ConVariableCached& operator=(ConVariableCached&& Other) noexcept;

    virtual int32 GetVal() const override;
    virtual void SetVal(int32 NewVal) override;
    int32 GetCache() const;
    void SetCache(int32 NewVal);
    // swaps the value and the cache
    void Swap();
    ConVariableCached SwapInPlace();

    ConVariable* GetCacheVariable();
    const ConVariable* GetCacheVariable() const;

private:
    struct CacheProxy : public ConVariable
    {
        explicit CacheProxy(ConVariableCached& InOwner)
            : Owner(InOwner) {}

        virtual int32 GetVal() const override { return Owner.GetCache(); }
        virtual void SetVal(int32 NewVal) override { Owner.SetCache(NewVal); }

    private:
        ConVariableCached& Owner;
    };

    CacheProxy CacheAccessor;
    int32 Val = 0;
    int32 Cache = 0;
};

struct ConVariableList : public vector<ConVariable>
{
    
};
#pragma once
#include "common.h"

enum class VariableKind
{
    Thread,
    Cache,
    List,
    Literal
};

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

struct VariableRef
{
    VariableRef() = default;
    VariableRef(VariableKind InKind, ConVariable* InPtr, ConVariableCached* InOwner = nullptr);

    static VariableRef ThreadVar(ConVariableCached* Var);
    static VariableRef CacheVar(ConVariableCached* Var);
    static VariableRef ListVar(ConVariableList* Var);
    static VariableRef LiteralVar(ConVariableAbsolute* Var);

    bool IsValid() const { return Ptr != nullptr; }
    VariableKind GetKind() const { return Kind; }
    bool IsThread() const { return Kind == VariableKind::Thread; }
    bool IsCache() const { return Kind == VariableKind::Cache; }
    bool IsList() const { return Kind == VariableKind::List; }
    bool IsLiteral() const { return Kind == VariableKind::Literal; }
    bool TouchesThread() const { return Kind == VariableKind::Thread || Kind == VariableKind::Cache; }

    ConVariable* GetVariable() const { return Ptr; }
    ConVariableCached* GetThread() const { return IsThread() ? ThreadOwner : nullptr; }
    ConVariableCached* GetThreadOwner() const { return ThreadOwner; }
    ConVariableList* GetList() const;
    ConVariableAbsolute* GetLiteral() const;

    int32 Read() const;
    void Write(int32 Value) const;

private:
    VariableKind Kind = VariableKind::Literal;
    ConVariable* Ptr = nullptr;
    ConVariableCached* ThreadOwner = nullptr;
};

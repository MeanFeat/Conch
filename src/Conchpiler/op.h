#pragma once
#include "common.h"
#include "compilable.h"
#include "errors.h"
#include "variable.h"

#include <vector>

struct ThreadMixSummary
{
    int32 UniqueThreadVars = 0;
    bool HasCacheReads = false;
    bool UsesLiteral = false;
};

struct ConBaseOp : public ConCompilable
{
    ConBaseOp() = default;
    explicit ConBaseOp(const vector<VariableRef>& InArgs);
    virtual ~ConBaseOp() override {};
    virtual void SetArgs(vector<VariableRef> Args);
    virtual int32 GetMaxArgs() const { return 2; }
    virtual bool HasReturn() const { return false; }
    virtual VariableRef GetReturn() const;
    const vector<VariableRef>& GetArgs() const;
    int32 GetArgsCount() const { return int32(GetArgs().size()); }

    void SetSourceLocation(const ConSourceLocation& InLocation) { Location = InLocation; }
    const ConSourceLocation& GetSourceLocation() const { return Location; }

    virtual void UpdateCycleCount() override;
    virtual void UpdateCycleCount(int32 VarCount);
    virtual int32 GetBaseCycleCost() const { return 1; }
    virtual int32 GetVariableAccessCount() const;
    const ThreadMixSummary& GetThreadMixSummary() const { return MixSummary; }
    bool MixesMultipleThreadVariables() const;
    std::vector<ConVariableCached*> GetThreadParticipants() const;

protected:
    vector<VariableRef>& GetMutableArgs();
    VariableRef& GetArgRef(int32 Index);
    const VariableRef& GetArgRef(int32 Index) const;
    void RefreshThreadMixSummary();

private:
    vector<VariableRef> Args;
    ConSourceLocation Location;
    ThreadMixSummary MixSummary;
};

// if has return operate on last 2 args and place result in arg[0] if in-place, operate on 2 args and replace value of first
struct ConContextualReturnOp : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 3; }
    virtual bool HasReturn() const override { return GetArgsCount() > 2; }
    virtual void Execute() override {}
    VariableRef& GetDstArg();
    const VariableRef& GetDstArg() const;
    vector<VariableRef> GetSrcArg() const;
};

enum class ConBinaryOpKind
{
    Add,
    Sub,
    Mul,
    Div,
    And,
    Or,
    Xor
};

struct ConBinaryOp final : public ConContextualReturnOp
{
    ConBinaryOp(ConBinaryOpKind InKind, const std::vector<VariableRef>& InArgs);
    virtual void Execute() override;

private:
    ConBinaryOpKind Kind;
    bool bHasPrecomputed = false;
    int32 PrecomputedValue = 0;
    int32 Compute(int32 Lhs, int32 Rhs) const;
};

struct ConIncrOp final : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 1; }
    virtual void Execute() override;
};

struct ConDecrOp final : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 1; }
    virtual void Execute() override;
};

struct ConNotOp final : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 2; }
    virtual void Execute() override;
};

struct ConPopOp final : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 2; }
    virtual bool HasReturn() const override { return true; }
    virtual void Execute() override;
};

struct ConAtOp final : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 3; }
    virtual bool HasReturn() const override { return true; }
    virtual void Execute() override;
};

struct ConSetOp final : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 2; }
    virtual bool HasReturn() const override { return false; }
    virtual void Execute() override;
};

struct ConSwpOp final : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 1; }
    virtual bool HasReturn() const override { return false; }
    virtual void Execute() override;
};



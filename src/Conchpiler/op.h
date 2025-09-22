#pragma once
#include "common.h"
#include "compilable.h"
#include "variable.h"

struct ConBaseOp : public ConCompilable
{
    ConBaseOp() = default;
    explicit ConBaseOp( const vector<ConVariable*> &InArgs) : Args(InArgs) {}
    virtual ~ConBaseOp() override {};
    virtual void SetArgs(vector<ConVariable*> Args);
    virtual int32 GetMaxArgs() const { return 2; }
    virtual bool HasReturn() const { return false; }
    virtual ConVariable* GetReturn() const;
    const vector<ConVariable*> &GetArgs() const;
    int32 GetArgsCount() const { return int32(GetArgs().size()); }

    virtual void UpdateCycleCount() override;
    virtual void UpdateCycleCount(int32 VarCount);
    virtual int32 GetBaseCycleCost() const { return 1; }
    virtual int32 GetVariableAccessCount() const;

    template<typename T>
    T GetArgAs(int32 Index);

private:
    vector<ConVariable*> Args;
};

template <typename T>
T ConBaseOp::GetArgAs(const int32 Index)
{
    return static_cast<T>(GetArgs().at(Index));
}

// if has return operate on last 2 args and place result in arg[0] if in-place, operate on 2 args and replace value of first 
struct ConContextualReturnOp : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 3; }
    virtual bool HasReturn() const override { return GetArgsCount() > 2; }
    virtual void Execute() override {}
    ConVariable* GetDstArg() const;
    vector<const ConVariable*> GetSrcArg() const;
};

struct ConAddOp final : public ConContextualReturnOp
{
    using ConContextualReturnOp::ConContextualReturnOp;
    virtual void Execute() override;
};

struct ConMulOp final : public ConContextualReturnOp
{
    using ConContextualReturnOp::ConContextualReturnOp;
    virtual void Execute() override;
};

struct ConSubOp final : public ConContextualReturnOp
{
    using ConContextualReturnOp::ConContextualReturnOp;
    virtual void Execute() override;
};

struct ConDivOp final : public ConContextualReturnOp
{
    using ConContextualReturnOp::ConContextualReturnOp;
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



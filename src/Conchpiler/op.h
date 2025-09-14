#pragma once
#include "common.h"
#include "compilable.h"
#include "variable.h"

enum class OperandKind { Reg, Imm };

struct Operand
{
    OperandKind kind;
    int32 value;
};

inline int32 ReadOperand(const Operand& o, const vector<ConVariable*>& regs)
{
    switch (o.kind)
    {
    case OperandKind::Imm:
        return o.value;
    case OperandKind::Reg:
        assert(o.value >= 0 && o.value < (int32)regs.size());
        return regs[o.value]->GetVal();
    }
    return 0;
}

struct ConBaseOp : public ConCompilable
{
    ConBaseOp() = default;
    explicit ConBaseOp(const vector<Operand> &InArgs) : Args(InArgs) {}
    virtual ~ConBaseOp() override {};
    virtual void SetArgs(vector<Operand> Args);
    virtual int32 GetMaxArgs() const { return 2; }
    virtual bool HasReturn() const { return false; }
    virtual Operand GetReturn() const;
    const vector<Operand> &GetArgs() const;
    int32 GetArgsCount() const { return int32(GetArgs().size()); }

    template<typename T>
    T GetArgAs(int32 Index);

private:
    vector<Operand> Args;
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
    virtual void Execute(vector<ConVariable*>& regs) override {}
    Operand GetDstArg() const;
    vector<Operand> GetSrcArg() const;
};

struct ConAddOp final : public ConContextualReturnOp
{
    using ConContextualReturnOp::ConContextualReturnOp;
    virtual void Execute(vector<ConVariable*>& regs) override;
};

struct ConMulOp final : public ConContextualReturnOp
{
    using ConContextualReturnOp::ConContextualReturnOp;
    virtual void Execute(vector<ConVariable*>& regs) override;
};

struct ConSubOp final : public ConContextualReturnOp
{
    using ConContextualReturnOp::ConContextualReturnOp;
    virtual void Execute(vector<ConVariable*>& regs) override;
};

struct ConSetOp final : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 2; }
    virtual bool HasReturn() const override { return false; }
    virtual void Execute(vector<ConVariable*>& regs) override;
};

struct ConSwpOp final : public ConBaseOp
{
    using ConBaseOp::ConBaseOp;
    virtual int32 GetMaxArgs() const override { return 1; }
    virtual bool HasReturn() const override { return false; }
    virtual void Execute(vector<ConVariable*>& regs) override;
};



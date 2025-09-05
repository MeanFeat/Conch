#pragma once
#include "common.h"
#include "compilable.h"
#include "variable.h"


struct ConBaseOp : public ConCompilable
{
    virtual ~ConBaseOp() override = default;
    virtual void SetArgs(vector<ConVariable*> Args);
    virtual int32 GetMaxArgs() const { return 2; }
    virtual bool HasReturn() const { return false; }
    virtual ConVariable* GetReturn() const;
    const vector<ConVariable*> &GetArgs() const;
    int32 GetArgsCount() const { return GetArgs().size(); }

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

struct ConSetOp : public ConBaseOp
{
    virtual ~ConSetOp() override = default;
    virtual int32 GetMaxArgs() const override { return 2; }
    virtual bool HasReturn() const override { return false; }
    virtual void Execute() override;
};

struct ConSwpOp : public ConBaseOp
{
    virtual ~ConSwpOp() override = default;
    virtual int32 GetMaxArgs() const override { return 1; }
    virtual bool HasReturn() const override { return false; }
    virtual void Execute() override;
};

struct ConAddOp : public ConBaseOp
{
    virtual ~ConAddOp() override = default;
    virtual int32 GetMaxArgs() const override { return 3; }
    virtual bool HasReturn() const override { return GetArgsCount() > 2; }
    virtual void Execute() override;
};

#pragma once
#include "common.h"
#include "compilable.h"
#include "op.h"
#include "variable.h"

enum class ConConditionOp
{
    None,
    GTR,
    LSR,
    EQL
};

struct ConLine : public ConCompilable
{
public:

    virtual  ~ConLine() override;
    virtual void Execute() override;
    virtual void UpdateCycleCount() override;
    void UpdateCycleCount(int32 VarCount);
    void SetOps(const vector<ConBaseOp*>& Ops);
    void SetCondition(ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, int32 SkipCount, bool bInvert);
    bool IsConditional() const { return Condition != ConConditionOp::None; }
    bool EvaluateCondition() const;
    int32 GetSkipCount() const { return Skip; }

private:
    // in reverse order of operation
    vector<ConBaseOp*> Ops;
    ConConditionOp Condition = ConConditionOp::None;
    ConVariable* Left = nullptr;
    ConVariable* Right = nullptr;
    int32 Skip = 0;
    bool Invert = false;
};


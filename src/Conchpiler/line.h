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

enum class ConLineKind
{
    Ops,
    If,
    Loop,
    Redo,
    Jump
};

struct ConLine : public ConCompilable
{
public:

    virtual  ~ConLine() override;
    virtual void Execute() override;
    virtual void UpdateCycleCount() override;
    void UpdateCycleCount(int32 VarCount);
    void SetOps(const vector<ConBaseOp*>& InOps, ConSourceLocation InLocation);
    void SetIf(ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, int32 SkipCount, bool bInvert, ConSourceLocation InLocation);
    void SetLoop(ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, bool bInvert, int32 ExitIndex, ConSourceLocation InLocation);
    void SetRedo(int32 TargetIndex, ConVariableCached* CounterVar, bool bInfinite, ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, bool bInvert, ConSourceLocation InLocation);
    void SetJump(int32 TargetIndex, ConConditionOp Op, ConVariable* Lhs, ConVariable* Rhs, bool bInvert, ConSourceLocation InLocation);

    ConLineKind GetKind() const { return Kind; }
    bool HasCondition() const { return Condition != ConConditionOp::None; }
    bool EvaluateCondition() const;
    int32 GetSkipCount() const { return Skip; }
    int32 GetLoopExitIndex() const { return LoopExitIndex; }
    int32 GetTargetIndex() const { return TargetIndex; }
    ConVariableCached* GetCounterVar() const { return Counter; }
    bool HasCounter() const { return Counter != nullptr; }
    bool IsInfiniteLoop() const { return bInfiniteLoop; }
    const ConSourceLocation& GetLocation() const { return Location; }

private:
    // in reverse order of operation
    vector<ConBaseOp*> Ops;
    ConLineKind Kind = ConLineKind::Ops;
    ConConditionOp Condition = ConConditionOp::None;
    ConVariable* Left = nullptr;
    ConVariable* Right = nullptr;
    int32 Skip = 0;
    int32 LoopExitIndex = -1;
    int32 TargetIndex = -1;
    ConVariableCached* Counter = nullptr;
    bool bInfiniteLoop = false;
    bool Invert = false;
    ConSourceLocation Location;
};


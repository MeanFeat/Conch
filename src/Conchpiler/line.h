#pragma once
#include "common.h"
#include "compilable.h"
#include "op.h"
#include "variable.h"

#include <string>

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
    void SetIf(ConConditionOp Op, VariableRef Lhs, VariableRef Rhs, int32 SkipCount, bool bInvert, ConSourceLocation InLocation);
    void SetLoop(ConConditionOp Op, VariableRef Lhs, VariableRef Rhs, bool bInvert, int32 ExitIndex, ConSourceLocation InLocation);
    void SetRedo(int32 TargetIndex, VariableRef CounterVar, bool bInfinite, ConConditionOp Op, VariableRef Lhs, VariableRef Rhs, bool bInvert, ConSourceLocation InLocation);
    void SetJump(int32 TargetIndex, ConConditionOp Op, VariableRef Lhs, VariableRef Rhs, bool bInvert, ConSourceLocation InLocation);

    ConLineKind GetKind() const { return Kind; }
    bool HasCondition() const { return Condition != ConConditionOp::None; }
    bool EvaluateCondition() const;
    int32 GetSkipCount() const { return Skip; }
    int32 GetLoopExitIndex() const { return LoopExitIndex; }
    int32 GetTargetIndex() const { return TargetIndex; }
    const VariableRef& GetCounter() const { return Counter; }
    bool HasCounter() const { return Counter.IsThread(); }
    bool IsInfiniteLoop() const { return bInfiniteLoop; }
    const ConSourceLocation& GetLocation() const { return Location; }
    void SetSourceText(const std::string& Text) { SourceText = Text; }
    const std::string& GetSourceText() const { return SourceText; }

private:
    // in reverse order of operation
    vector<ConBaseOp*> Ops;
    ConLineKind Kind = ConLineKind::Ops;
    ConConditionOp Condition = ConConditionOp::None;
    VariableRef Left;
    VariableRef Right;
    int32 Skip = 0;
    int32 LoopExitIndex = -1;
    int32 TargetIndex = -1;
    VariableRef Counter;
    bool bInfiniteLoop = false;
    bool Invert = false;
    ConSourceLocation Location;
    std::string SourceText;
};


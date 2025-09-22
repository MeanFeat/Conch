#pragma once
#include "line.h"
#include "variable.h"
#include <memory>
#include <string>
#include <vector>

struct ConThread final : public ConCompilable
{
public:
    ConThread(): Variables() {}
    explicit ConThread(const vector<ConVariable*> &InVariables) : Variables(InVariables) {}
    virtual void Execute() override;
    virtual void UpdateCycleCount() override;
    void SetVariables(const vector<ConVariable*>& InVariables);
    void SetOwnedStorage(std::vector<std::unique_ptr<ConVariableCached>>&& CachedVars,
                         std::vector<std::unique_ptr<ConVariableAbsolute>>&& ConstVars,
                         std::vector<std::unique_ptr<ConVariableList>>&& ListVars,
                         std::vector<std::unique_ptr<ConBaseOp>>&& Ops);
    void ConstructLine(const ConLine& Line);

    bool HadRuntimeError() const { return bHadRuntimeError; }
    const std::vector<std::string>& GetRuntimeErrors() const { return RuntimeErrors; }

private:
    void ReportRuntimeError(const ConRuntimeError& Error);
    void ResetRuntimeErrors();

    vector<ConVariable*> Variables;
    vector<ConLine> Lines;
    std::vector<std::unique_ptr<ConVariableCached>> OwnedVarStorage;
    std::vector<std::unique_ptr<ConVariableAbsolute>> OwnedConstStorage;
    std::vector<std::unique_ptr<ConVariableList>> OwnedListStorage;
    std::vector<std::unique_ptr<ConBaseOp>> OwnedOpStorage;
    std::vector<std::string> RuntimeErrors;
    bool bHadRuntimeError = false;
};

#pragma once
#include "line.h"
#include "variable.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ConThread final : public ConCompilable
{
public:
    ConThread() : ThreadVariables() {}
    explicit ConThread(const vector<ConVariableCached*>& InVariables) : ThreadVariables(InVariables) {}
    virtual void Execute() override;
    virtual void UpdateCycleCount() override;
    void SetVariables(const vector<ConVariableCached*>& InVariables);
    void SetOwnedStorage(std::vector<std::unique_ptr<ConVariableCached>>&& CachedVars,
                         std::vector<std::unique_ptr<ConVariableAbsolute>>&& ConstVars,
                         std::vector<std::unique_ptr<ConVariableList>>&& ListVars,
                         std::vector<std::unique_ptr<ConBaseOp>>&& Ops,
                         std::unordered_map<std::string, ConVariableList*>&& ListNameMap);
    void ConstructLine(const ConLine& Line);

    bool HadRuntimeError() const { return bHadRuntimeError; }
    const std::vector<std::string>& GetRuntimeErrors() const { return RuntimeErrors; }

    bool DidReturn() const { return bDidReturn; }
    bool HasReturnValue() const { return bDidReturn && bReturnHasValue; }
    int32 GetReturnValue() const { return ReturnValue; }

    void SetTraceEnabled(bool bEnabled);
    bool IsTraceEnabled() const { return bTraceExecution; }

    size_t GetThreadVarCount() const { return ThreadVariables.size(); }
    ConVariableCached* GetThreadVar(size_t Index);
    const ConVariableCached* GetThreadVar(size_t Index) const;
    int32 GetThreadValue(size_t Index) const;
    int32 GetThreadCacheValue(size_t Index) const;
    void SetThreadValue(size_t Index, int32 Value);

    size_t GetListVarCount() const { return OwnedListStorage.size(); }
    ConVariableList* GetListVar(size_t Index);
    const ConVariableList* GetListVar(size_t Index) const;
    ConVariableList* FindListVar(const std::string& Name);
    const ConVariableList* FindListVar(const std::string& Name) const;
    std::string GetListName(const ConVariableList* List) const;
    std::vector<std::string> GetListNames() const;

private:
    void ReportRuntimeError(const ConRuntimeError& Error);
    void ResetRuntimeErrors();

    vector<ConVariableCached*> ThreadVariables;
    vector<ConLine> Lines;
    std::vector<std::unique_ptr<ConVariableCached>> OwnedVarStorage;
    std::vector<std::unique_ptr<ConVariableAbsolute>> OwnedConstStorage;
    std::vector<std::unique_ptr<ConVariableList>> OwnedListStorage;
    std::vector<std::unique_ptr<ConBaseOp>> OwnedOpStorage;
    std::unordered_map<std::string, ConVariableList*> ListLookup;
    std::unordered_map<ConVariableList*, std::string> ReverseListLookup;
    std::vector<std::string> RuntimeErrors;
    bool bHadRuntimeError = false;
    bool bTraceExecution = true;
    bool bDidReturn = false;
    bool bReturnHasValue = false;
    int32 ReturnValue = 0;
};

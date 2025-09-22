#pragma once

#include "errors.h"
#include "scanner.h"
#include "thread.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ConParser
{
    ConParser();

    bool Parse(const vector<string>& Lines, ConThread& OutThread);
    const std::vector<std::string>& GetErrors() const { return Errors; }
    bool HadError() const { return bHadError; }

private:
    void Reset();

    std::vector<std::unique_ptr<ConVariableCached>> VarStorage;
    std::vector<std::unique_ptr<ConVariableAbsolute>> ConstStorage;
    std::vector<std::unique_ptr<ConVariableList>> ListStorage;
    std::vector<std::unique_ptr<ConBaseOp>> OpStorage;
    std::unordered_map<std::string, VariableRef> VarMap;
    std::vector<std::string> Errors;
    bool bHadError = false;

    VariableRef ResolveToken(const Token& Tok);
    std::vector<ConBaseOp*> ParseTokens(const std::vector<Token>& Tokens);
    void ReportError(const Token& Tok, const std::string& Message);
    void ReportError(const ConParseError& Error);
};


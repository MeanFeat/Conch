#pragma once

#include "thread.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ConParser
{
    ConParser();

    ConThread Parse(const vector<string>& Lines);

private:
    std::vector<std::unique_ptr<ConVariableCached>> VarStorage;
    std::vector<std::unique_ptr<ConVariableAbsolute>> ConstStorage;
    std::vector<std::unique_ptr<ConBaseOp>> OpStorage;
    std::unordered_map<std::string, ConVariableCached*> VarMap;

    ConVariable* ResolveToken(const std::string& Tok);
    std::vector<ConBaseOp*> ParseTokens(const std::vector<std::string>& Tokens);
};


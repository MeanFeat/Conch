#pragma once

#include "common.h"

#include <optional>
#include <string>
#include <vector>

enum class TokenType
{
    Identifier,
    Number,
    Colon,
    EndOfLine
};

struct Token
{
    TokenType Type = TokenType::Identifier;
    std::string Lexeme;
    std::optional<int32> Literal;
    int32 Line = 0;
    int32 Column = 0;
};

struct TokenLine
{
    int32 Indent = 0;
    std::vector<Token> Tokens;
};

class Scanner
{
public:
    explicit Scanner(const std::vector<std::string>& InLines);

    std::vector<TokenLine> Scan();
    const std::vector<std::string>& GetErrors() const { return Errors; }

private:
    void ScanLine(const std::string& LineText, int32 LineNumber, TokenLine& OutLine);
    void AddError(int32 LineNumber, int32 ColumnNumber, const std::string& Message);

    std::vector<std::string> Lines;
    std::vector<std::string> Errors;
};


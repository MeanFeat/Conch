#include "scanner.h"

#include <cctype>
#include <sstream>

namespace
{
bool IsIdentifierStart(char C)
{
    return std::isalpha(static_cast<unsigned char>(C)) != 0 || C == '_';
}

bool IsIdentifierPart(char C)
{
    return std::isalnum(static_cast<unsigned char>(C)) != 0 || C == '_';
}
}

Scanner::Scanner(const std::vector<std::string>& InLines)
    : Lines(InLines)
{
}

std::vector<TokenLine> Scanner::Scan()
{
    std::vector<TokenLine> Result;
    Result.reserve(Lines.size());
    for (size_t Index = 0; Index < Lines.size(); ++Index)
    {
        TokenLine Line;
        ScanLine(Lines[Index], static_cast<int32>(Index + 1), Line);
        Result.push_back(std::move(Line));
    }
    return Result;
}

void Scanner::ScanLine(const std::string& LineText, const int32 LineNumber, TokenLine& OutLine)
{
    size_t Position = 0;
    int32 Indent = 0;
    while (Position < LineText.size() && (LineText[Position] == ' ' || LineText[Position] == '\t'))
    {
        Indent += (LineText[Position] == '\t') ? 4 : 1;
        ++Position;
    }
    OutLine.Indent = Indent;

    while (Position < LineText.size())
    {
        char Current = LineText[Position];
        if (Current == ' ' || Current == '\t')
        {
            ++Position;
            continue;
        }

        const int32 Column = static_cast<int32>(Position + 1);

        if (IsIdentifierStart(Current))
        {
            size_t Start = Position;
            while (Position < LineText.size() && IsIdentifierPart(LineText[Position]))
            {
                ++Position;
            }
            Token Tok;
            Tok.Type = TokenType::Identifier;
            Tok.Lexeme = LineText.substr(Start, Position - Start);
            Tok.Line = LineNumber;
            Tok.Column = Column;
            OutLine.Tokens.push_back(std::move(Tok));
            continue;
        }

        if (Current == '-' || std::isdigit(static_cast<unsigned char>(Current)) != 0)
        {
            size_t Start = Position;
            if (Current == '-')
            {
                ++Position;
                if (Position >= LineText.size() || std::isdigit(static_cast<unsigned char>(LineText[Position])) == 0)
                {
                    AddError(LineNumber, Column, "Standalone '-' is not a valid literal");
                    continue;
                }
            }
            while (Position < LineText.size() && std::isdigit(static_cast<unsigned char>(LineText[Position])) != 0)
            {
                ++Position;
            }
            const std::string Lexeme = LineText.substr(Start, Position - Start);
            Token Tok;
            Tok.Type = TokenType::Number;
            Tok.Lexeme = Lexeme;
            Tok.Line = LineNumber;
            Tok.Column = Column;
            try
            {
                Tok.Literal = std::stoi(Lexeme);
                Tok.bHasLiteral = true;
            }
            catch (const std::exception&)
            {
                AddError(LineNumber, Column, "Numeric literal out of range");
                Tok.bHasLiteral = false;
                Tok.Literal = 0;
            }
            OutLine.Tokens.push_back(std::move(Tok));
            continue;
        }

        if (Current == ':')
        {
            Token Tok;
            Tok.Type = TokenType::Colon;
            Tok.Lexeme = ":";
            Tok.Line = LineNumber;
            Tok.Column = Column;
            OutLine.Tokens.push_back(std::move(Tok));
            ++Position;
            continue;
        }

        AddError(LineNumber, Column, std::string("Unexpected character '") + Current + "'");
        ++Position;
    }
}

void Scanner::AddError(const int32 LineNumber, const int32 ColumnNumber, const std::string& Message)
{
    std::ostringstream Stream;
    Stream << "[line " << LineNumber << ", col " << ColumnNumber << "] " << Message;
    Errors.push_back(Stream.str());
}


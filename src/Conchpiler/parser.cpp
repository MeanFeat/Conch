#include "parser.h"
#include <sstream>
#include <cctype>

ConParser::ConParser()
{
    VarStorage.emplace_back(std::make_unique<ConVariableCached>());
    VarMap["X"] = VarStorage.back().get();
    VarStorage.emplace_back(std::make_unique<ConVariableCached>());
    VarMap["Y"] = VarStorage.back().get();
    VarStorage.emplace_back(std::make_unique<ConVariableCached>());
    VarMap["Z"] = VarStorage.back().get();
}

ConVariable* ConParser::ResolveToken(const std::string& Tok)
{
    auto It = VarMap.find(Tok);
    if (It != VarMap.end())
    {
        return It->second;
    }
    int32 Val = std::stoi(Tok);
    ConstStorage.emplace_back(std::make_unique<ConVariableAbsolute>(Val));
    return ConstStorage.back().get();
}

std::vector<ConBaseOp*> ConParser::ParseTokens(const std::vector<std::string>& Tokens)
{
    std::vector<ConBaseOp*> Ops;
    std::vector<ConVariable*> Stack;

    for (int32 i = static_cast<int32>(Tokens.size()) - 1; i >= 0; --i)
    {
        const std::string& Tok = Tokens.at(i);
        if (Tok == "SET")
        {
            ConVariable* Dst = Stack.back(); Stack.pop_back();
            ConVariable* Src = Stack.back(); Stack.pop_back();
            OpStorage.emplace_back(std::make_unique<ConSetOp>(std::vector<ConVariable*>{Dst, Src}));
            Ops.push_back(OpStorage.back().get());
            Stack.push_back(Dst);
        }
        else if (Tok == "SWP")
        {
            ConVariable* Var = Stack.back(); Stack.pop_back();
            OpStorage.emplace_back(std::make_unique<ConSwpOp>(std::vector<ConVariable*>{Var}));
            Ops.push_back(OpStorage.back().get());
            Stack.push_back(Var);
        }
        else if (Tok == "ADD" || Tok == "SUB" || Tok == "MUL" || Tok == "DIV")
        {
            // handle expressions of the form: SET <dst> OP <a> <b>
            if (i >= 2 && Tokens.at(i - 2) == "SET")
            {
                ConVariable* SrcA = Stack.back(); Stack.pop_back();
                ConVariable* SrcB = Stack.back(); Stack.pop_back();
                ConVariable* Dst = ResolveToken(Tokens.at(i - 1));
                if (Tok == "ADD")
                {
                    OpStorage.emplace_back(std::make_unique<ConAddOp>(std::vector<ConVariable*>{Dst, SrcA, SrcB}));
                }
                else if (Tok == "SUB")
                {
                    OpStorage.emplace_back(std::make_unique<ConSubOp>(std::vector<ConVariable*>{Dst, SrcA, SrcB}));
                }
                else if (Tok == "MUL")
                {
                    OpStorage.emplace_back(std::make_unique<ConMulOp>(std::vector<ConVariable*>{Dst, SrcA, SrcB}));
                }
                else
                {
                    OpStorage.emplace_back(std::make_unique<ConDivOp>(std::vector<ConVariable*>{Dst, SrcA, SrcB}));
                }
                Ops.push_back(OpStorage.back().get());
                Stack.push_back(Dst);
                i -= 2; // consume destination and SET tokens
            }
            else
            {
                ConVariable* Dst = Stack.back(); Stack.pop_back();
                ConVariable* Src = Stack.back(); Stack.pop_back();
                if (Tok == "ADD")
                {
                    OpStorage.emplace_back(std::make_unique<ConAddOp>(std::vector<ConVariable*>{Dst, Src}));
                }
                else if (Tok == "SUB")
                {
                    OpStorage.emplace_back(std::make_unique<ConSubOp>(std::vector<ConVariable*>{Dst, Src}));
                }
                else if (Tok == "MUL")
                {
                    OpStorage.emplace_back(std::make_unique<ConMulOp>(std::vector<ConVariable*>{Dst, Src}));
                }
                else
                {
                    OpStorage.emplace_back(std::make_unique<ConDivOp>(std::vector<ConVariable*>{Dst, Src}));
                }
                Ops.push_back(OpStorage.back().get());
                Stack.push_back(Dst);
            }
        }
        else
        {
            Stack.push_back(ResolveToken(Tok));
        }
    }

    return Ops;
}

struct ParsedLine
{
    int32 Indent = 0;
    bool IsIf = false;
    ConConditionOp Cmp = ConConditionOp::None;
    ConVariable* Lhs = nullptr;
    ConVariable* Rhs = nullptr;
    int32 SkipCount = 0;
    bool Invert = false;
    std::vector<ConBaseOp*> Ops;
};

ConThread ConParser::Parse(const std::vector<std::string>& Lines)
{
    std::vector<ParsedLine> Parsed;
    for (const std::string& RawLine : Lines)
    {
        ParsedLine P;
        size_t Pos = 0;
        while (Pos < RawLine.size() && isspace(static_cast<unsigned char>(RawLine[Pos])))
        {
            ++Pos;
        }
        P.Indent = static_cast<int32>(Pos);
        std::string Line = RawLine.substr(Pos);
        std::stringstream SS(Line);
        std::vector<std::string> Tokens;
        std::string Tok;
        while (SS >> Tok)
        {
            Tokens.push_back(Tok);
        }
        if (!Tokens.empty() && (Tokens[0] == "IF" || Tokens[0] == "IFN"))
        {
            P.IsIf = true;
            P.Invert = Tokens[0] == "IFN";
            if (Tokens[1] == "GTR")
            {
                P.Cmp = ConConditionOp::GTR;
            }
            else if (Tokens[1] == "LSR")
            {
                P.Cmp = ConConditionOp::LSR;
            }
            else
            {
                P.Cmp = ConConditionOp::EQL;
            }
            P.Lhs = ResolveToken(Tokens[2]);
            P.Rhs = ResolveToken(Tokens[3]);
        }
        else
        {
            P.Ops = ParseTokens(Tokens);
        }
        Parsed.push_back(P);
    }

    std::vector<int32> Stack;
    for (int32 i = 0; i < static_cast<int32>(Parsed.size()); ++i)
    {
        while (!Stack.empty() && Parsed[i].Indent <= Parsed[Stack.back()].Indent)
        {
            int32 Idx = Stack.back();
            Stack.pop_back();
            Parsed[Idx].SkipCount = i - Idx - 1;
        }
        if (Parsed[i].IsIf)
        {
            Stack.push_back(i);
        }
    }
    while (!Stack.empty())
    {
        int32 Idx = Stack.back();
        Stack.pop_back();
        Parsed[Idx].SkipCount = static_cast<int32>(Parsed.size()) - Idx - 1;
    }

    std::vector<ConVariable*> Vars;
    for (auto& V : VarStorage)
    {
        Vars.push_back(V.get());
    }
    ConThread Thread(Vars);
    for (const ParsedLine& P : Parsed)
    {
        ConLine Line;
        Line.SetOps(P.Ops);
        if (P.IsIf)
        {
            Line.SetCondition(P.Cmp, P.Lhs, P.Rhs, P.SkipCount, P.Invert);
        }
        Thread.ConstructLine(Line);
    }
    return Thread;
}


#include "parser.h"

#include <array>
#include <sstream>
#include <cctype>
#include <unordered_map>

ConParser::ConParser()
{
    const std::array<std::string, 3> BaseVars = {"X", "Y", "Z"};
    for (const std::string& Name : BaseVars)
    {
        VarStorage.emplace_back(std::make_unique<ConVariableCached>());
        ConVariableCached* Var = VarStorage.back().get();
        VarMap[Name] = Var;
        VarMap[Name + "C"] = Var->GetCacheVariable();
    }
}

ConVariable* ConParser::ResolveToken(const std::string& Tok)
{
    auto It = VarMap.find(Tok);
    if (It != VarMap.end())
    {
        return It->second;
    }
    if (Tok.rfind("LIST", 0) == 0)
    {
        const std::string IndexStr = Tok.substr(4);
        const int32 Index = std::stoi(IndexStr);
        while (static_cast<int32>(ListStorage.size()) <= Index)
        {
            ListStorage.emplace_back(std::make_unique<ConVariableList>());
            const std::string Name = "LIST" + std::to_string(static_cast<int32>(ListStorage.size()) - 1);
            VarMap[Name] = ListStorage.back().get();
        }
        VarMap[Tok] = ListStorage.at(Index).get();
        return ListStorage.at(Index).get();
    }
    const int32 Val = std::stoi(Tok);
    ConstStorage.emplace_back(std::make_unique<ConVariableAbsolute>(Val));
    return ConstStorage.back().get();
}

std::vector<ConBaseOp*> ConParser::ParseTokens(const std::vector<std::string>& Tokens)
{
    std::vector<ConBaseOp*> Ops;
    std::vector<ConVariable*> Stack;

    auto AddOp = [&](std::unique_ptr<ConBaseOp> Op, ConVariable* Result)
    {
        OpStorage.emplace_back(std::move(Op));
        Ops.push_back(OpStorage.back().get());
        Stack.push_back(Result);
    };

    auto PopValue = [&]() -> ConVariable*
    {
        assert(!Stack.empty());
        ConVariable* Value = Stack.back();
        Stack.pop_back();
        return Value;
    };

    auto PopCached = [&]() -> ConVariableCached*
    {
        ConVariable* Value = PopValue();
        ConVariableCached* Cached = dynamic_cast<ConVariableCached*>(Value);
        assert(Cached != nullptr);
        return Cached;
    };

    auto IsInlineSet = [&](int32 Index) -> bool
    {
        return Index >= 2 && Tokens.at(Index - 2) == "SET";
    };

    auto ResolveInlineDestination = [&](int32 Index) -> ConVariableCached*
    {
        ConVariable* RawDst = ResolveToken(Tokens.at(Index - 1));
        ConVariableCached* Dst = dynamic_cast<ConVariableCached*>(RawDst);
        assert(Dst != nullptr);
        return Dst;
    };

    static const std::unordered_map<std::string, ConBinaryOpKind> BinaryOpMap =
    {
        {"ADD", ConBinaryOpKind::Add},
        {"SUB", ConBinaryOpKind::Sub},
        {"MUL", ConBinaryOpKind::Mul},
        {"DIV", ConBinaryOpKind::Div},
        {"AND", ConBinaryOpKind::And},
        {"OR", ConBinaryOpKind::Or},
        {"XOR", ConBinaryOpKind::Xor}
    };

    for (int32 i = static_cast<int32>(Tokens.size()) - 1; i >= 0; --i)
    {
        const std::string& Tok = Tokens.at(i);
        auto BinaryIt = BinaryOpMap.find(Tok);

        if (Tok == "SET")
        {
            ConVariableCached* Dst = PopCached();
            ConVariable* Src = PopValue();
            AddOp(std::make_unique<ConSetOp>(std::vector<ConVariable*>{Dst, Src}), Dst);
        }
        else if (Tok == "SWP")
        {
            ConVariableCached* Var = PopCached();
            AddOp(std::make_unique<ConSwpOp>(std::vector<ConVariable*>{Var}), Var);
        }
        else if (BinaryIt != BinaryOpMap.end())
        {
            const ConBinaryOpKind Kind = BinaryIt->second;
            if (IsInlineSet(i))
            {
                ConVariable* SrcA = PopValue();
                ConVariable* SrcB = PopValue();
                ConVariableCached* Dst = ResolveInlineDestination(i);
                AddOp(std::make_unique<ConBinaryOp>(Kind, std::vector<ConVariable*>{Dst, SrcA, SrcB}), Dst);
                i -= 2;
            }
            else
            {
                ConVariableCached* Dst = PopCached();
                ConVariable* Src = PopValue();
                AddOp(std::make_unique<ConBinaryOp>(Kind, std::vector<ConVariable*>{Dst, Src}), Dst);
            }
        }
        else if (Tok == "INCR" || Tok == "DECR")
        {
            ConVariableCached* Dst = PopCached();
            if (Tok == "INCR")
            {
                AddOp(std::make_unique<ConIncrOp>(std::vector<ConVariable*>{Dst}), Dst);
            }
            else
            {
                AddOp(std::make_unique<ConDecrOp>(std::vector<ConVariable*>{Dst}), Dst);
            }
        }
        else if (Tok == "NOT")
        {
            if (IsInlineSet(i))
            {
                ConVariable* Src = PopValue();
                ConVariableCached* Dst = ResolveInlineDestination(i);
                AddOp(std::make_unique<ConNotOp>(std::vector<ConVariable*>{Dst, Src}), Dst);
                i -= 2;
            }
            else
            {
                ConVariableCached* Dst = PopCached();
                AddOp(std::make_unique<ConNotOp>(std::vector<ConVariable*>{Dst}), Dst);
            }
        }
        else if (Tok == "POP")
        {
            if (IsInlineSet(i))
            {
                ConVariable* List = PopValue();
                ConVariableCached* Dst = ResolveInlineDestination(i);
                AddOp(std::make_unique<ConPopOp>(std::vector<ConVariable*>{Dst, List}), Dst);
                i -= 2;
            }
            else
            {
                ConVariableCached* Dst = PopCached();
                ConVariable* List = PopValue();
                AddOp(std::make_unique<ConPopOp>(std::vector<ConVariable*>{Dst, List}), Dst);
            }
        }
        else if (Tok == "AT")
        {
            if (IsInlineSet(i))
            {
                ConVariable* List = PopValue();
                ConVariable* Index = PopValue();
                ConVariableCached* Dst = ResolveInlineDestination(i);
                AddOp(std::make_unique<ConAtOp>(std::vector<ConVariable*>{Dst, List, Index}), Dst);
                i -= 2;
            }
            else
            {
                ConVariableCached* Dst = PopCached();
                ConVariable* List = PopValue();
                ConVariable* Index = PopValue();
                AddOp(std::make_unique<ConAtOp>(std::vector<ConVariable*>{Dst, List, Index}), Dst);
            }
        }
        else
        {
            Stack.push_back(ResolveToken(Tok));
        }
    }

    return Ops;
}

enum class ParsedLineType
{
    Ops,
    If,
    Loop,
    Redo,
    Jump
};

struct ParsedLine
{
    int32 Indent = 0;
    std::string Label;
    ParsedLineType Type = ParsedLineType::Ops;
    ConConditionOp Cmp = ConConditionOp::None;
    ConVariable* Lhs = nullptr;
    ConVariable* Rhs = nullptr;
    bool Invert = false;
    int32 SkipCount = 0;
    int32 LoopExitIndex = -1;
    int32 TargetIndex = -1;
    std::string TargetLabel;
    ConVariableCached* Counter = nullptr;
    bool InfiniteLoop = false;
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
        if (Tokens.empty())
        {
            Parsed.push_back(P);
            continue;
        }

        if (!Tokens.empty() && Tokens[0].back() == ':' && Tokens[0].size() > 1)
        {
            P.Label = Tokens[0].substr(0, Tokens[0].size() - 1);
            Tokens.erase(Tokens.begin());
        }

        if (Tokens.empty())
        {
            Parsed.push_back(P);
            continue;
        }

        auto ParseComparisonToken = [](const std::string& Comp) -> ConConditionOp
        {
            if (Comp == "GTR")
            {
                return ConConditionOp::GTR;
            }
            if (Comp == "LSR")
            {
                return ConConditionOp::LSR;
            }
            return ConConditionOp::EQL;
        };
        auto IsComparison = [](const std::string& Comp) -> bool
        {
            return Comp == "GTR" || Comp == "LSR" || Comp == "EQL";
        };

        const std::string& Command = Tokens[0];
        if (Command == "IF" || Command == "IFN")
        {
            P.Type = ParsedLineType::If;
            P.Invert = Command == "IFN";
            assert(Tokens.size() >= 4);
            P.Cmp = ParseComparisonToken(Tokens[1]);
            P.Lhs = ResolveToken(Tokens[2]);
            P.Rhs = ResolveToken(Tokens[3]);
        }
        else if (Command == "LOOP")
        {
            P.Type = ParsedLineType::Loop;
            if (Tokens.size() >= 4 && IsComparison(Tokens[1]))
            {
                P.Cmp = ParseComparisonToken(Tokens[1]);
                P.Lhs = ResolveToken(Tokens[2]);
                P.Rhs = ResolveToken(Tokens[3]);
            }
        }
        else if (Command == "REDO")
        {
            P.Type = ParsedLineType::Redo;
            if (Tokens.size() == 1)
            {
                P.InfiniteLoop = true;
            }
            else if (Tokens.size() >= 4 && IsComparison(Tokens[1]))
            {
                P.Cmp = ParseComparisonToken(Tokens[1]);
                P.Lhs = ResolveToken(Tokens[2]);
                P.Rhs = ResolveToken(Tokens[3]);
            }
            else
            {
                ConVariable* CounterVar = ResolveToken(Tokens[1]);
                P.Counter = dynamic_cast<ConVariableCached*>(CounterVar);
                assert(P.Counter != nullptr && "REDO counter must be a cached variable");
            }
        }
        else if (Command == "JUMP")
        {
            P.Type = ParsedLineType::Jump;
            size_t LabelIndex = 1;
            if (Tokens.size() >= 5 && IsComparison(Tokens[1]))
            {
                P.Cmp = ParseComparisonToken(Tokens[1]);
                P.Lhs = ResolveToken(Tokens[2]);
                P.Rhs = ResolveToken(Tokens[3]);
                LabelIndex = 4;
            }
            assert(LabelIndex < Tokens.size() && "JUMP requires a label target");
            P.TargetLabel = Tokens[LabelIndex];
        }
        else
        {
            P.Ops = ParseTokens(Tokens);
        }
        Parsed.push_back(P);
    }

    std::vector<int32> IfStack;
    for (int32 i = 0; i < static_cast<int32>(Parsed.size()); ++i)
    {
        while (!IfStack.empty() && Parsed[i].Indent <= Parsed[IfStack.back()].Indent)
        {
            int32 Idx = IfStack.back();
            IfStack.pop_back();
            Parsed[Idx].SkipCount = i - Idx - 1;
        }
        if (Parsed[i].Type == ParsedLineType::If)
        {
            IfStack.push_back(i);
        }
    }
    while (!IfStack.empty())
    {
        int32 Idx = IfStack.back();
        IfStack.pop_back();
        Parsed[Idx].SkipCount = static_cast<int32>(Parsed.size()) - Idx - 1;
    }

    for (int32 i = 0; i < static_cast<int32>(Parsed.size()); ++i)
    {
        if (Parsed[i].Type == ParsedLineType::Redo)
        {
            int32 Match = -1;
            for (int32 j = i - 1; j >= 0; --j)
            {
                if (Parsed[j].Type == ParsedLineType::Loop && Parsed[j].Indent <= Parsed[i].Indent)
                {
                    Match = j;
                    break;
                }
            }
            assert(Match >= 0 && "REDO must have a matching LOOP");
            Parsed[i].TargetIndex = Match;
            Parsed[Match].LoopExitIndex = i + 1;
        }
    }

    for (ParsedLine& P : Parsed)
    {
        if (P.Type == ParsedLineType::Loop && P.LoopExitIndex < 0)
        {
            P.LoopExitIndex = static_cast<int32>(Parsed.size());
        }
    }

    std::unordered_map<std::string, int32> LabelMap;
    for (int32 i = 0; i < static_cast<int32>(Parsed.size()); ++i)
    {
        if (!Parsed[i].Label.empty())
        {
            LabelMap[Parsed[i].Label] = i;
        }
    }

    for (ParsedLine& P : Parsed)
    {
        if (P.Type == ParsedLineType::Jump)
        {
            auto ItLabel = LabelMap.find(P.TargetLabel);
            assert(ItLabel != LabelMap.end() && "JUMP target label not found");
            P.TargetIndex = ItLabel->second;
        }
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
        switch (P.Type)
        {
        case ParsedLineType::Ops:
            Line.SetOps(P.Ops);
            break;
        case ParsedLineType::If:
            Line.SetIf(P.Cmp, P.Lhs, P.Rhs, P.SkipCount, P.Invert);
            break;
        case ParsedLineType::Loop:
            Line.SetLoop(P.Cmp, P.Lhs, P.Rhs, P.Invert, P.LoopExitIndex);
            break;
        case ParsedLineType::Redo:
            Line.SetRedo(P.TargetIndex, P.Counter, P.InfiniteLoop, P.Cmp, P.Lhs, P.Rhs, P.Invert);
            break;
        case ParsedLineType::Jump:
            Line.SetJump(P.TargetIndex, P.Cmp, P.Lhs, P.Rhs, P.Invert);
            break;
        }
        Thread.ConstructLine(Line);
    }
    return Thread;
}


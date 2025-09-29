#include "parser.h"

#include <array>
#include <sstream>
#include <unordered_map>

namespace
{
ConConditionOp ParseComparisonToken(const std::string& Comp)
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
}

bool IsComparisonToken(const std::string& Comp)
{
    return Comp == "GTR" || Comp == "LSR" || Comp == "EQL";
}
}

ConParser::ConParser()
{
    Reset();
}

void ConParser::Reset()
{
    VarStorage.clear();
    ConstStorage.clear();
    ListStorage.clear();
    OpStorage.clear();
    VarMap.clear();

    const std::array<std::string, 3> BaseVars = {"X", "Y", "Z"};
    for (const std::string& Name : BaseVars)
    {
        VarStorage.emplace_back(std::make_unique<ConVariableCached>());
        ConVariableCached* Var = VarStorage.back().get();
        VarMap[Name] = VariableRef::ThreadVar(Var);
        VarMap[Name + "C"] = VariableRef::CacheVar(Var);
    }
}

void ConParser::ReportError(const Token& Tok, const std::string& Message)
{
    ConSourceLocation Location;
    Location.Line = Tok.Line;
    Location.Column = Tok.Column;
    bHadError = true;
    Errors.push_back(FormatErrorMessage(Location, Message));
}

void ConParser::ReportError(const ConParseError& Error)
{
    bHadError = true;
    Errors.push_back(FormatErrorMessage(Error.Location, Error.what()));
}

VariableRef ConParser::ResolveToken(const Token& Tok)
{
    if (Tok.Type == TokenType::Number)
    {
        if (!Tok.bHasLiteral)
        {
            throw ConParseError(Tok, "Numeric token missing literal value");
        }
        ConstStorage.emplace_back(std::make_unique<ConVariableAbsolute>(Tok.Literal));
        return VariableRef::LiteralVar(ConstStorage.back().get());
    }

    const std::string& Lexeme = Tok.Lexeme;
    auto It = VarMap.find(Lexeme);
    if (It != VarMap.end())
    {
        return It->second;
    }
    if (Lexeme.rfind("LIST", 0) == 0)
    {
        const std::string IndexStr = Lexeme.substr(4);
        if (IndexStr.empty())
        {
            throw ConParseError(Tok, "LIST token missing index");
        }
        int32 Index = 0;
        try
        {
            Index = std::stoi(IndexStr);
        }
        catch (const std::exception&)
        {
            throw ConParseError(Tok, "Invalid LIST index");
        }
        while (static_cast<int32>(ListStorage.size()) <= Index)
        {
            ListStorage.emplace_back(std::make_unique<ConVariableList>());
            const std::string Name = "LIST" + std::to_string(static_cast<int32>(ListStorage.size()) - 1);
            VarMap[Name] = VariableRef::ListVar(ListStorage.back().get());
        }
        VariableRef Ref = VariableRef::ListVar(ListStorage.at(Index).get());
        VarMap[Lexeme] = Ref;
        return Ref;
    }

    try
    {
        const int32 Val = std::stoi(Lexeme);
        ConstStorage.emplace_back(std::make_unique<ConVariableAbsolute>(Val));
        return VariableRef::LiteralVar(ConstStorage.back().get());
    }
    catch (const std::exception&)
    {
        throw ConParseError(Tok, "Unable to resolve token '" + Lexeme + "'");
    }
}

std::vector<ConBaseOp*> ConParser::ParseTokens(const std::vector<Token>& Tokens)
{
    std::vector<ConBaseOp*> Ops;
    const size_t InitialOpStorageSize = OpStorage.size();
    struct StackEntry
    {
        VariableRef Value;
        Token TokenInfo;
    };

    std::vector<StackEntry> Stack;

    auto PushEntry = [&](const VariableRef& Ref, const Token& Tok)
    {
        StackEntry Entry;
        Entry.Value = Ref;
        Entry.TokenInfo = Tok;
        Stack.push_back(Entry);
    };

    auto PopValue = [&](const Token& Context) -> StackEntry
    {
        if (Stack.empty())
        {
            throw ConParseError(Context, "Not enough values before '" + Context.Lexeme + "'");
        }
        StackEntry Entry = Stack.back();
        Stack.pop_back();
        if (!Entry.Value.IsValid())
        {
            throw ConParseError(Entry.TokenInfo, "Invalid value before '" + Context.Lexeme + "'");
        }
        return Entry;
    };

    auto PopThread = [&](const Token& Context) -> StackEntry
    {
        StackEntry Entry = PopValue(Context);
        if (!Entry.Value.IsThread())
        {
            throw ConParseError(Entry.TokenInfo, "Expected thread variable");
        }
        return Entry;
    };

    auto IsInlineSet = [&](int32 Index) -> bool
    {
        return Index >= 2 && Tokens.at(Index - 2).Type == TokenType::Identifier && Tokens.at(Index - 2).Lexeme == "SET";
    };

    auto ResolveInlineDestination = [&](int32 Index) -> StackEntry
    {
        const Token& DestToken = Tokens.at(Index - 1);
        VariableRef Dst = ResolveToken(DestToken);
        if (!Dst.IsThread())
        {
            throw ConParseError(DestToken, "Inline destination must be a thread variable");
        }
        StackEntry Entry;
        Entry.Value = Dst;
        Entry.TokenInfo = DestToken;
        return Entry;
    };

    auto StoreOp = [&](std::unique_ptr<ConBaseOp> Op, const StackEntry& ResultEntry, const Token& OpToken)
    {
        Op->SetSourceLocation({OpToken.Line, OpToken.Column});
        OpStorage.emplace_back(std::move(Op));
        ConBaseOp* Stored = OpStorage.back().get();
        Ops.push_back(Stored);
        Stack.push_back(ResultEntry);
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

    try
    {
        for (int32 i = static_cast<int32>(Tokens.size()) - 1; i >= 0; --i)
        {
            const Token& Tok = Tokens.at(i);
            if (Tok.Type == TokenType::Colon)
            {
                continue;
            }

            if (Tok.Type == TokenType::Identifier)
            {
                auto BinaryIt = BinaryOpMap.find(Tok.Lexeme);

                if (Tok.Lexeme == "SET")
                {
                    StackEntry DstEntry = PopThread(Tok);
                    StackEntry SrcEntry = PopValue(Tok);
                    std::vector<VariableRef> Args = {DstEntry.Value, SrcEntry.Value};
                    StoreOp(std::make_unique<ConSetOp>(Args), DstEntry, Tok);
                }
                else if (Tok.Lexeme == "SWP")
                {
                    StackEntry VarEntry = PopThread(Tok);
                    std::vector<VariableRef> Args = {VarEntry.Value};
                    StoreOp(std::make_unique<ConSwpOp>(Args), VarEntry, Tok);
                }
                else if (BinaryIt != BinaryOpMap.end())
                {
                    const ConBinaryOpKind Kind = BinaryIt->second;
                    if (IsInlineSet(i))
                    {
                        StackEntry SrcA = PopValue(Tok);
                        StackEntry SrcB = PopValue(Tok);
                        StackEntry DstEntry = ResolveInlineDestination(i);
                        std::vector<VariableRef> Args = {DstEntry.Value, SrcA.Value, SrcB.Value};
                        StoreOp(std::make_unique<ConBinaryOp>(Kind, Args), DstEntry, Tok);
                        i -= 2;
                    }
                    else
                    {
                        StackEntry DstEntry = PopThread(Tok);
                        StackEntry SrcEntry = PopValue(Tok);
                        std::vector<VariableRef> Args = {DstEntry.Value, SrcEntry.Value};
                        StoreOp(std::make_unique<ConBinaryOp>(Kind, Args), DstEntry, Tok);
                    }
                }
                else if (Tok.Lexeme == "INCR" || Tok.Lexeme == "DECR")
                {
                    StackEntry DstEntry = PopThread(Tok);
                    if (Tok.Lexeme == "INCR")
                    {
                        std::vector<VariableRef> Args = {DstEntry.Value};
                        StoreOp(std::make_unique<ConIncrOp>(Args), DstEntry, Tok);
                    }
                    else
                    {
                        std::vector<VariableRef> Args = {DstEntry.Value};
                        StoreOp(std::make_unique<ConDecrOp>(Args), DstEntry, Tok);
                    }
                }
                else if (Tok.Lexeme == "NOT")
                {
                    if (IsInlineSet(i))
                    {
                        StackEntry SrcEntry = PopValue(Tok);
                        StackEntry DstEntry = ResolveInlineDestination(i);
                        std::vector<VariableRef> Args = {DstEntry.Value, SrcEntry.Value};
                        StoreOp(std::make_unique<ConNotOp>(Args), DstEntry, Tok);
                        i -= 2;
                    }
                    else
                    {
                        StackEntry DstEntry = PopThread(Tok);
                        std::vector<VariableRef> Args = {DstEntry.Value};
                        StoreOp(std::make_unique<ConNotOp>(Args), DstEntry, Tok);
                    }
                }
                else if (Tok.Lexeme == "POP")
                {
                    if (IsInlineSet(i))
                    {
                        StackEntry ListEntry = PopValue(Tok);
                        StackEntry DstEntry = ResolveInlineDestination(i);
                        std::vector<VariableRef> Args = {DstEntry.Value, ListEntry.Value};
                        StoreOp(std::make_unique<ConPopOp>(Args), DstEntry, Tok);
                        i -= 2;
                    }
                    else
                    {
                        StackEntry DstEntry = PopThread(Tok);
                        StackEntry ListEntry = PopValue(Tok);
                        std::vector<VariableRef> Args = {DstEntry.Value, ListEntry.Value};
                        StoreOp(std::make_unique<ConPopOp>(Args), DstEntry, Tok);
                    }
                }
                else if (Tok.Lexeme == "AT")
                {
                    if (IsInlineSet(i))
                    {
                        StackEntry ListEntry = PopValue(Tok);
                        StackEntry IndexEntry = PopValue(Tok);
                        StackEntry DstEntry = ResolveInlineDestination(i);
                        std::vector<VariableRef> Args = {DstEntry.Value, ListEntry.Value, IndexEntry.Value};
                        StoreOp(std::make_unique<ConAtOp>(Args), DstEntry, Tok);
                        i -= 2;
                    }
                    else
                    {
                        StackEntry DstEntry = PopThread(Tok);
                        StackEntry ListEntry = PopValue(Tok);
                        StackEntry IndexEntry = PopValue(Tok);
                        std::vector<VariableRef> Args = {DstEntry.Value, ListEntry.Value, IndexEntry.Value};
                        StoreOp(std::make_unique<ConAtOp>(Args), DstEntry, Tok);
                    }
                }
                else
                {
                    VariableRef Value = ResolveToken(Tok);
                    PushEntry(Value, Tok);
                }
            }
            else
            {
                VariableRef Value = ResolveToken(Tok);
                PushEntry(Value, Tok);
            }
        }
    }
    catch (const ConParseError& Error)
    {
        ReportError(Error);
        while (OpStorage.size() > InitialOpStorageSize)
        {
            OpStorage.pop_back();
        }
        Ops.clear();
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
    VariableRef Lhs;
    VariableRef Rhs;
    bool Invert = false;
    int32 SkipCount = 0;
    int32 LoopExitIndex = -1;
    int32 TargetIndex = -1;
    std::string TargetLabel;
    VariableRef Counter;
    bool InfiniteLoop = false;
    std::vector<ConBaseOp*> Ops;
    ConSourceLocation Location;
    std::string SourceText;
};

bool ConParser::Parse(const std::vector<std::string>& Lines, ConThread& OutThread)
{
    Reset();
    Errors.clear();
    bHadError = false;

    Scanner Tokenizer(Lines);
    std::vector<TokenLine> TokenLines = Tokenizer.Scan();
    const std::vector<std::string>& ScanErrors = Tokenizer.GetErrors();
    Errors.insert(Errors.end(), ScanErrors.begin(), ScanErrors.end());
    if (!ScanErrors.empty())
    {
        bHadError = true;
        return false;
    }

    std::vector<ParsedLine> Parsed;
    Parsed.reserve(TokenLines.size());

    for (size_t LineIndex = 0; LineIndex < TokenLines.size(); ++LineIndex)
    {
        const TokenLine& LineTokens = TokenLines[LineIndex];
        ParsedLine P;
        P.Indent = LineTokens.Indent;
        if (LineIndex < Lines.size())
        {
            P.SourceText = Lines[LineIndex];
        }

        std::vector<Token> Tokens = LineTokens.Tokens;

        if (Tokens.size() >= 2 && Tokens[0].Type == TokenType::Identifier && Tokens[1].Type == TokenType::Colon)
        {
            P.Label = Tokens[0].Lexeme;
            Tokens.erase(Tokens.begin(), Tokens.begin() + 2);
        }

        if (Tokens.empty())
        {
            Parsed.push_back(std::move(P));
            continue;
        }

        const Token& CommandToken = Tokens[0];
        P.Location = {CommandToken.Line, CommandToken.Column};
        const std::string& Command = CommandToken.Lexeme;

        auto EnsureArgs = [&](size_t Count, const std::string& Message) -> bool
        {
            if (Tokens.size() < Count)
            {
                ReportError(CommandToken, Message);
                return false;
            }
            return true;
        };

        try
        {
            if (Command == "IF" || Command == "IFN")
            {
                if (!EnsureArgs(4, "IF requires a comparison and two operands"))
                {
                    Parsed.push_back(std::move(P));
                    continue;
                }
                P.Type = ParsedLineType::If;
                P.Invert = Command == "IFN";
                P.Cmp = ParseComparisonToken(Tokens[1].Lexeme);
                P.Lhs = ResolveToken(Tokens[2]);
                P.Rhs = ResolveToken(Tokens[3]);
            }
            else if (Command == "LOOP")
            {
                P.Type = ParsedLineType::Loop;
                if (Tokens.size() >= 4 && Tokens[1].Type == TokenType::Identifier && IsComparisonToken(Tokens[1].Lexeme))
                {
                    P.Cmp = ParseComparisonToken(Tokens[1].Lexeme);
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
                else if (Tokens.size() >= 4 && Tokens[1].Type == TokenType::Identifier && IsComparisonToken(Tokens[1].Lexeme))
                {
                    P.Cmp = ParseComparisonToken(Tokens[1].Lexeme);
                    P.Lhs = ResolveToken(Tokens[2]);
                    P.Rhs = ResolveToken(Tokens[3]);
                }
                else
                {
                    VariableRef CounterVar = ResolveToken(Tokens[1]);
                    if (!CounterVar.IsThread())
                    {
                        throw ConParseError(Tokens[1], "REDO counter must be a thread variable");
                    }
                    P.Counter = CounterVar;
                }
            }
            else if (Command == "JUMP")
            {
                P.Type = ParsedLineType::Jump;
                size_t LabelIndex = 1;
                if (Tokens.size() >= 5 && Tokens[1].Type == TokenType::Identifier && IsComparisonToken(Tokens[1].Lexeme))
                {
                    P.Cmp = ParseComparisonToken(Tokens[1].Lexeme);
                    P.Lhs = ResolveToken(Tokens[2]);
                    P.Rhs = ResolveToken(Tokens[3]);
                    LabelIndex = 4;
                }
                if (LabelIndex < Tokens.size())
                {
                    P.TargetLabel = Tokens[LabelIndex].Lexeme;
                }
                else
                {
                    ReportError(CommandToken, "JUMP requires a label target");
                }
            }
            else
            {
                P.Type = ParsedLineType::Ops;
                P.Ops = ParseTokens(Tokens);
            }
        }
        catch (const ConParseError& Error)
        {
            ReportError(Error);
        }

        Parsed.push_back(std::move(P));
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
            if (Match < 0)
            {
                ReportError(Token{}, "REDO must have a matching LOOP");
            }
            else
            {
                Parsed[i].TargetIndex = Match;
                Parsed[Match].LoopExitIndex = i + 1;
            }
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
            if (ItLabel == LabelMap.end())
            {
                ReportError(Token{}, "JUMP target label not found: " + P.TargetLabel);
            }
            else
            {
                P.TargetIndex = ItLabel->second;
            }
        }
    }

    if (bHadError)
    {
        return false;
    }

    std::vector<ConVariableCached*> Vars;
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
            Line.SetOps(P.Ops, P.Location);
            break;
        case ParsedLineType::If:
            Line.SetIf(P.Cmp, P.Lhs, P.Rhs, P.SkipCount, P.Invert, P.Location);
            break;
        case ParsedLineType::Loop:
            Line.SetLoop(P.Cmp, P.Lhs, P.Rhs, P.Invert, P.LoopExitIndex, P.Location);
            break;
        case ParsedLineType::Redo:
            Line.SetRedo(P.TargetIndex, P.Counter, P.InfiniteLoop, P.Cmp, P.Lhs, P.Rhs, P.Invert, P.Location);
            break;
        case ParsedLineType::Jump:
            Line.SetJump(P.TargetIndex, P.Cmp, P.Lhs, P.Rhs, P.Invert, P.Location);
            break;
        }
        Line.SetSourceText(P.SourceText);
        Thread.ConstructLine(Line);
    }
    Thread.SetOwnedStorage(std::move(VarStorage), std::move(ConstStorage), std::move(ListStorage), std::move(OpStorage));
    OutThread = std::move(Thread);
    return true;
}


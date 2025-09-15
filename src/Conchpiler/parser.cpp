#include "parser.h"
#include <sstream>

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

    for (auto It = Tokens.rbegin(); It != Tokens.rend(); ++It)
    {
        const std::string& Tok = *It;
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
        else
        {
            Stack.push_back(ResolveToken(Tok));
        }
    }

    return Ops;
}

ConThread ConParser::Parse(const std::vector<std::string>& Lines)
{
    std::vector<ConVariable*> Vars;
    for (auto& V : VarStorage)
    {
        Vars.push_back(V.get());
    }
    ConThread Thread(Vars);
    for (const std::string& Line : Lines)
    {
        std::stringstream SS(Line);
        std::vector<std::string> Tokens;
        std::string Tok;
        while (SS >> Tok)
        {
            Tokens.push_back(Tok);
        }
        Thread.ConstructLine(ParseTokens(Tokens));
    }
    return Thread;
}


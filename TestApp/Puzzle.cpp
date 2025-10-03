#include "Puzzle.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <initializer_list>
#include <sstream>

namespace
{
std::string ToUpper(const std::string& Text)
{
    std::string Result = Text;
    std::transform(Result.begin(), Result.end(), Result.begin(), [](unsigned char Ch) { return static_cast<char>(std::toupper(Ch)); });
    return Result;
}

const SimpleJsonValue* FindFirstOf(const SimpleJsonValue& Object, std::initializer_list<const char*> Keys)
{
    for (const char* Key : Keys)
    {
        if (const SimpleJsonValue* Value = Object.Find(Key))
        {
            return Value;
        }
    }
    return nullptr;
}

bool ExtractStringArray(const SimpleJsonValue& Value, std::vector<std::string>& OutStrings, std::string& OutError)
{
    if (!Value.IsArray())
    {
        OutError = "Expected array of strings";
        return false;
    }
    for (const SimpleJsonValue& Entry : Value.GetArray())
    {
        if (!Entry.IsString())
        {
            OutError = "Expected string value in array";
            return false;
        }
        OutStrings.push_back(Entry.AsString());
    }
    return true;
}

bool ExtractListSpecs(const SimpleJsonValue& Value, std::vector<PuzzleListSpec>& OutSpecs, std::string& OutError, const std::string& ExpectedPrefix = std::string())
{
    if (!Value.IsArray())
    {
        OutError = "Expected array of list definitions";
        return false;
    }
    for (const SimpleJsonValue& Entry : Value.GetArray())
    {
        if (!Entry.IsObject())
        {
            OutError = "List definition must be an object";
            return false;
        }
        const SimpleJsonValue* NameValue = Entry.Find("name");
        const SimpleJsonValue* ValuesValue = Entry.Find("values");
        if (NameValue == nullptr || !NameValue->IsString())
        {
            OutError = "List definition missing string 'name'";
            return false;
        }
        if (ValuesValue == nullptr || !ValuesValue->IsArray())
        {
            OutError = "List definition missing array 'values'";
            return false;
        }
        PuzzleListSpec Spec;
        Spec.Name = ToUpper(NameValue->AsString());
        if (!ExpectedPrefix.empty() && Spec.Name.rfind(ExpectedPrefix, 0) != 0)
        {
            OutError = std::string("Expected list name starting with '") + ExpectedPrefix + "'";
            return false;
        }
        for (const SimpleJsonValue& Number : ValuesValue->GetArray())
        {
            if (!Number.IsNumber())
            {
                OutError = "List values must be numeric";
                return false;
            }
            Spec.Values.push_back(Number.AsInt());
        }
        OutSpecs.push_back(Spec);
    }
    return true;
}

bool ExtractOutSpecs(const SimpleJsonValue& Value, std::vector<PuzzleOutSpec>& OutSpecs, std::string& OutError)
{
    if (!Value.IsArray())
    {
        OutError = "Expected array for OUT specifications";
        return false;
    }
    for (const SimpleJsonValue& Entry : Value.GetArray())
    {
        if (!Entry.IsObject())
        {
            OutError = "OUT specification must be an object";
            return false;
        }
        const SimpleJsonValue* NameValue = Entry.Find("name");
        const SimpleJsonValue* SizeValue = FindFirstOf(Entry, {"expectedSize", "size"});
        if (NameValue == nullptr || !NameValue->IsString())
        {
            OutError = "OUT specification missing string 'name'";
            return false;
        }
        if (SizeValue == nullptr || !SizeValue->IsNumber())
        {
            OutError = "OUT specification missing numeric 'expectedSize'";
            return false;
        }
        const int Size = SizeValue->AsInt();
        if (Size < 0)
        {
            OutError = "OUT expected size must be non-negative";
            return false;
        }
        PuzzleOutSpec Spec;
        Spec.Name = ToUpper(NameValue->AsString());
        if (Spec.Name.rfind("OUT", 0) != 0)
        {
            OutError = "OUT specification names must start with 'OUT'";
            return false;
        }
        Spec.ExpectedSize = Size;
        OutSpecs.push_back(Spec);
    }
    return true;
}

bool ExtractRegisterMap(const SimpleJsonValue& Value, std::unordered_map<std::string, int>& OutRegisters, std::string& OutError)
{
    if (!Value.IsObject())
    {
        OutError = "Register block must be an object";
        return false;
    }
    for (const auto& Pair : Value.GetObject())
    {
        if (!Pair.second.IsNumber())
        {
            OutError = "Register values must be numeric";
            return false;
        }
        OutRegisters[ToUpper(Pair.first)] = Pair.second.AsInt();
    }
    return true;
}
}

bool ParseRegisterName(const std::string& Name, int& OutIndex)
{
    const std::string Upper = ToUpper(Name);
    if (Upper == "X")
    {
        OutIndex = 0;
        return true;
    }
    if (Upper == "Y")
    {
        OutIndex = 1;
        return true;
    }
    if (Upper == "Z")
    {
        OutIndex = 2;
        return true;
    }
    return false;
}

bool LoadPuzzleFromFile(const std::string& Path, PuzzleData& OutData, std::string& OutError)
{
    std::ifstream Input(Path);
    if (!Input)
    {
        OutError = "Failed to open puzzle file: " + Path;
        return false;
    }
    std::ostringstream Buffer;
    Buffer << Input.rdbuf();

    SimpleJsonValue Root;
    if (!SimpleJsonValue::Parse(Buffer.str(), Root, OutError))
    {
        return false;
    }
    if (!Root.IsObject())
    {
        OutError = "Puzzle file must start with a JSON object";
        return false;
    }

    OutData = PuzzleData{};
    if (const SimpleJsonValue* Title = Root.Find("title"))
    {
        if (!Title->IsString())
        {
            OutError = "Puzzle title must be a string";
            return false;
        }
        OutData.Title = Title->AsString();
    }
    else
    {
        OutData.Title = "Untitled Puzzle";
    }

    if (const SimpleJsonValue* Description = Root.Find("description"))
    {
        if (!Description->IsString())
        {
            OutError = "Puzzle description must be a string";
            return false;
        }
        OutData.Description = Description->AsString();
    }

    if (const SimpleJsonValue* Starter = FindFirstOf(Root, {"starter", "starterCode", "naiveSolution"}))
    {
        if (!ExtractStringArray(*Starter, OutData.StarterCode, OutError))
        {
            return false;
        }
    }

    if (OutData.StarterCode.empty())
    {
        OutData.StarterCode = {"RET"};
    }

    if (const SimpleJsonValue* HistoryBlock = Root.Find("history"))
    {
        if (!HistoryBlock->IsArray())
        {
            OutError = "History must be an array";
            return false;
        }
        for (const SimpleJsonValue& Entry : HistoryBlock->GetArray())
        {
            if (!Entry.IsObject())
            {
                OutError = "History entries must be objects";
                return false;
            }
            const SimpleJsonValue* Label = Entry.Find("label");
            const SimpleJsonValue* Cycles = Entry.Find("cycles");
            if (Label == nullptr || !Label->IsString() || Cycles == nullptr || !Cycles->IsNumber())
            {
                OutError = "History entries require 'label' and numeric 'cycles'";
                return false;
            }
            PuzzleHistoryEntry HistoryEntry;
            HistoryEntry.Label = Label->AsString();
            HistoryEntry.Cycles = Cycles->AsInt();
            OutData.History.push_back(HistoryEntry);
        }
    }

    const SimpleJsonValue* Tests = Root.Find("tests");
    if (Tests == nullptr || !Tests->IsArray())
    {
        OutError = "Puzzle requires an array of tests";
        return false;
    }

    for (const SimpleJsonValue& TestValue : Tests->GetArray())
    {
        if (!TestValue.IsObject())
        {
            OutError = "Test entries must be objects";
            return false;
        }
        PuzzleTestCase Test;
        if (const SimpleJsonValue* NameValue = TestValue.Find("name"))
        {
            if (!NameValue->IsString())
            {
                OutError = "Test name must be a string";
                return false;
            }
            Test.Name = NameValue->AsString();
        }
        else
        {
            Test.Name = "Unnamed Test";
        }

        if (const SimpleJsonValue* Registers = TestValue.Find("registers"))
        {
            if (!ExtractRegisterMap(*Registers, Test.InitialRegisters, OutError))
            {
                return false;
            }
        }

        if (const SimpleJsonValue* DatInputs = FindFirstOf(TestValue, {"dat", "inputs"}))
        {
            if (!ExtractListSpecs(*DatInputs, Test.DatInputs, OutError, "DAT"))
            {
                return false;
            }
        }

        if (const SimpleJsonValue* OutSpecs = TestValue.Find("out"))
        {
            if (!ExtractOutSpecs(*OutSpecs, Test.OutSpecs, OutError))
            {
                return false;
            }
        }

        if (const SimpleJsonValue* ExpectedRegisters = TestValue.Find("expectedRegisters"))
        {
            if (!ExtractRegisterMap(*ExpectedRegisters, Test.Expectation.Registers, OutError))
            {
                return false;
            }
        }

        if (const SimpleJsonValue* ExpectedOut = FindFirstOf(TestValue, {"expectedOut", "expectedOutputs"}))
        {
            if (!ExtractListSpecs(*ExpectedOut, Test.Expectation.ExpectedOut, OutError, "OUT"))
            {
                return false;
            }
        }

        OutData.Tests.push_back(Test);
    }

    if (OutData.Tests.empty())
    {
        OutError = "Puzzle must define at least one test";
        return false;
    }

    return true;
}


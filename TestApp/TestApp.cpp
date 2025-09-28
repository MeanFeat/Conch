#include "Puzzle.h"

#include "../src/Conchpiler/parser.h"
#include "../src/Conchpiler/thread.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace
{
std::string Trim(const std::string& Text)
{
    const auto First = std::find_if_not(Text.begin(), Text.end(), [](unsigned char Ch) { return std::isspace(Ch); });
    const auto Last = std::find_if_not(Text.rbegin(), Text.rend(), [](unsigned char Ch) { return std::isspace(Ch); }).base();
    if (First >= Last)
    {
        return std::string();
    }
    return std::string(First, Last);
}

std::string PromptLine(const std::string& Message)
{
    std::cout << Message;
    std::string Line;
    std::getline(std::cin, Line);
    return Line;
}

std::string RegisterName(size_t Index)
{
    static const char* Names[] = {"X", "Y", "Z"};
    const size_t Count = sizeof(Names) / sizeof(Names[0]);
    if (Index < Count)
    {
        return Names[Index];
    }
    return "R" + std::to_string(Index);
}

std::string FormatListValues(const std::vector<int>& Values)
{
    std::ostringstream Oss;
    Oss << "[";
    for (size_t i = 0; i < Values.size(); ++i)
    {
        if (i > 0)
        {
            Oss << ", ";
        }
        Oss << Values[i];
    }
    Oss << "]";
    return Oss.str();
}

std::vector<std::pair<std::string, int>> SortRegisterMap(const std::unordered_map<std::string, int>& Registers)
{
    std::vector<std::pair<std::string, int>> Sorted(Registers.begin(), Registers.end());
    std::sort(Sorted.begin(), Sorted.end(), [](const auto& A, const auto& B)
    {
        return A.first < B.first;
    });
    return Sorted;
}

void ShowPuzzleOverview(const PuzzleData& Puzzle)
{
    std::cout << "=== " << Puzzle.Title << " ===\n";
    if (!Puzzle.Description.empty())
    {
        std::cout << Puzzle.Description << "\n";
    }
    std::cout << "\nTests:" << std::endl;
    for (size_t i = 0; i < Puzzle.Tests.size(); ++i)
    {
        const PuzzleTestCase& Test = Puzzle.Tests[i];
        std::cout << "  " << (i + 1) << ". " << Test.Name << std::endl;
        if (!Test.InitialRegisters.empty())
        {
            std::cout << "     Registers:";
            for (const auto& Pair : SortRegisterMap(Test.InitialRegisters))
            {
                std::cout << " " << Pair.first << "=" << Pair.second;
            }
            std::cout << std::endl;
        }
        if (!Test.InitialLists.empty())
        {
            std::cout << "     Lists:";
            for (const PuzzleListSpec& Spec : Test.InitialLists)
            {
                std::cout << " LIST" << Spec.Index << "=" << FormatListValues(Spec.Values);
            }
            std::cout << std::endl;
        }
        if (Test.Expectation.HasExpectations())
        {
            std::cout << "     Expectations:";
            for (const auto& Pair : SortRegisterMap(Test.Expectation.Registers))
            {
                std::cout << " " << Pair.first << "=" << Pair.second;
            }
            for (const PuzzleListSpec& Spec : Test.Expectation.Lists)
            {
                std::cout << " LIST" << Spec.Index << "=" << FormatListValues(Spec.Values);
            }
            std::cout << std::endl;
        }
    }
    if (!Puzzle.History.empty())
    {
        std::cout << "\nCycle history:" << std::endl;
        for (const PuzzleHistoryEntry& Entry : Puzzle.History)
        {
            std::cout << "  " << std::setw(4) << Entry.Cycles << " cycles - " << Entry.Label << std::endl;
        }
    }
    std::cout << std::endl;
}

void ShowCode(const std::vector<std::string>& Code)
{
    std::cout << "Current program (" << Code.size() << " line" << (Code.size() == 1 ? "" : "s") << "):\n";
    for (size_t i = 0; i < Code.size(); ++i)
    {
        std::cout << std::setw(3) << (i + 1) << " | " << Code[i] << std::endl;
    }
    std::cout << std::endl;
}

bool LoadCodeFromFile(const std::string& Path, std::vector<std::string>& OutCode, std::string& OutError)
{
    std::ifstream Input(Path);
    if (!Input)
    {
        OutError = "Unable to open file: " + Path;
        return false;
    }
    std::vector<std::string> Lines;
    std::string Line;
    while (std::getline(Input, Line))
    {
        Lines.push_back(Line);
    }
    OutCode = std::move(Lines);
    return true;
}

bool SaveCodeToFile(const std::string& Path, const std::vector<std::string>& Code, std::string& OutError)
{
    std::ofstream Output(Path);
    if (!Output)
    {
        OutError = "Unable to write to file: " + Path;
        return false;
    }
    for (size_t i = 0; i < Code.size(); ++i)
    {
        Output << Code[i];
        if (i + 1 < Code.size())
        {
            Output << '\n';
        }
    }
    return true;
}

void EditCode(std::vector<std::string>& Code)
{
    std::cout << "Enter new program lines. Type '.done' on a blank line to finish." << std::endl;
    std::vector<std::string> NewCode;
    while (true)
    {
        std::string Line;
        std::getline(std::cin, Line);
        if (!std::cin)
        {
            break;
        }
        const std::string Trimmed = Trim(Line);
        if (Trimmed == ".done")
        {
            break;
        }
        NewCode.push_back(Line);
    }
    if (!NewCode.empty())
    {
        Code = std::move(NewCode);
    }
}

bool ApplyTestSetup(const PuzzleTestCase& Test, ConThread& Thread, std::vector<std::string>& Messages)
{
    bool bSuccess = true;
    for (const auto& Pair : Test.InitialRegisters)
    {
        int RegisterIndex = 0;
        if (!ParseRegisterName(Pair.first, RegisterIndex))
        {
            Messages.push_back("Unknown register '" + Pair.first + "'");
            bSuccess = false;
            continue;
        }
        Thread.SetThreadValue(static_cast<size_t>(RegisterIndex), Pair.second);
    }
    for (const PuzzleListSpec& Spec : Test.InitialLists)
    {
        ConVariableList* List = Thread.GetListVar(static_cast<size_t>(Spec.Index));
        if (List == nullptr)
        {
            Messages.push_back("LIST" + std::to_string(Spec.Index) + " is not defined in this program");
            bSuccess = false;
            continue;
        }
        List->SetValues(Spec.Values);
        List->Reset();
    }
    return bSuccess;
}

std::vector<std::string> ValidateExpectations(const PuzzleExpectation& Expectation, const ConThread& Thread)
{
    std::vector<std::string> Issues;
    for (const auto& Pair : Expectation.Registers)
    {
        int RegisterIndex = 0;
        if (!ParseRegisterName(Pair.first, RegisterIndex))
        {
            Issues.push_back("Expectation references unknown register '" + Pair.first + "'");
            continue;
        }
        const int Actual = Thread.GetThreadValue(static_cast<size_t>(RegisterIndex));
        if (Actual != Pair.second)
        {
            Issues.push_back("Expected " + Pair.first + "=" + std::to_string(Pair.second) + ", got " + std::to_string(Actual));
        }
    }
    for (const PuzzleListSpec& Spec : Expectation.Lists)
    {
        const ConVariableList* List = Thread.GetListVar(static_cast<size_t>(Spec.Index));
        if (List == nullptr)
        {
            Issues.push_back("Expectation references undefined LIST" + std::to_string(Spec.Index));
            continue;
        }
        const std::vector<int32>& Actual = List->GetValues();
        const std::vector<int> ActualCopy(Actual.begin(), Actual.end());
        if (Actual.size() != Spec.Values.size() || !std::equal(Actual.begin(), Actual.end(), Spec.Values.begin()))
        {
            Issues.push_back("Expected LIST" + std::to_string(Spec.Index) + "=" + FormatListValues(Spec.Values) + ", got " + FormatListValues(ActualCopy));
        }
    }
    return Issues;
}

bool ComputeStaticCycleCount(const std::vector<std::string>& Code, int& OutCycles, std::vector<std::string>& Errors)
{
    ConParser Parser;
    ConThread Thread;
    if (!Parser.Parse(Code, Thread))
    {
        Errors = Parser.GetErrors();
        return false;
    }
    Thread.SetTraceEnabled(false);
    Thread.UpdateCycleCount();
    OutCycles = Thread.GetCycleCount();
    return true;
}

void PrintThreadState(const ConThread& Thread)
{
    std::cout << "    Registers:";
    for (size_t i = 0; i < Thread.GetThreadVarCount(); ++i)
    {
        std::cout << " " << RegisterName(i) << "=" << Thread.GetThreadValue(i) << " (C=" << Thread.GetThreadCacheValue(i) << ")";
    }
    std::cout << std::endl;
    if (Thread.GetListVarCount() > 0)
    {
        std::cout << "    Lists:";
        for (size_t i = 0; i < Thread.GetListVarCount(); ++i)
        {
            const ConVariableList* List = Thread.GetListVar(i);
            if (List != nullptr)
            {
                const std::vector<int32>& Values = List->GetValues();
                std::vector<int> Copy(Values.begin(), Values.end());
                std::cout << " LIST" << i << "=" << FormatListValues(Copy);
            }
        }
        std::cout << std::endl;
    }
}

void RunTests(const PuzzleData& Puzzle, const std::vector<std::string>& Code)
{
    if (Code.empty())
    {
        std::cout << "No code to execute. Use the editor or load a file first.\n";
        return;
    }

    std::vector<std::string> ParseErrors;
    int StaticCycles = 0;
    if (!ComputeStaticCycleCount(Code, StaticCycles, ParseErrors))
    {
        std::cout << "Syntax errors detected:\n";
        for (const std::string& Error : ParseErrors)
        {
            std::cout << "  " << Error << std::endl;
        }
        return;
    }

    std::cout << "Static cycle estimate: " << StaticCycles << std::endl;
    if (!Puzzle.History.empty())
    {
        const auto Best = std::min_element(Puzzle.History.begin(), Puzzle.History.end(), [](const PuzzleHistoryEntry& A, const PuzzleHistoryEntry& B)
        {
            return A.Cycles < B.Cycles;
        });
        if (Best != Puzzle.History.end())
        {
            if (StaticCycles < Best->Cycles)
            {
                std::cout << "You are on track to beat the recorded best of " << Best->Cycles << " cycles (" << Best->Label << ")!" << std::endl;
            }
            else if (StaticCycles == Best->Cycles)
            {
                std::cout << "Matched the best recorded cycle count of " << Best->Cycles << " (" << Best->Label << ")." << std::endl;
            }
            else
            {
                std::cout << "Need " << (StaticCycles - Best->Cycles) << " fewer cycles to beat the best record (" << Best->Label << ")." << std::endl;
            }
        }
    }
    std::cout << std::endl;

    bool bAllPassed = true;

    for (size_t TestIndex = 0; TestIndex < Puzzle.Tests.size(); ++TestIndex)
    {
        const PuzzleTestCase& Test = Puzzle.Tests[TestIndex];
        std::cout << "Running test " << (TestIndex + 1) << "/" << Puzzle.Tests.size() << ": " << Test.Name << std::endl;

        ConParser Parser;
        ConThread Thread;
        if (!Parser.Parse(Code, Thread))
        {
            std::cout << "  Parse failed during execution.\n";
            for (const std::string& Error : Parser.GetErrors())
            {
                std::cout << "    " << Error << std::endl;
            }
            bAllPassed = false;
            continue;
        }

        Thread.SetTraceEnabled(false);
        Thread.UpdateCycleCount();

        std::vector<std::string> SetupMessages;
        if (!ApplyTestSetup(Test, Thread, SetupMessages))
        {
            std::cout << "  Test setup failed:" << std::endl;
            for (const std::string& Message : SetupMessages)
            {
                std::cout << "    " << Message << std::endl;
            }
            bAllPassed = false;
            continue;
        }

        Thread.Execute();
        if (Thread.HadRuntimeError())
        {
            std::cout << "  Runtime error:" << std::endl;
            for (const std::string& Error : Thread.GetRuntimeErrors())
            {
                std::cout << "    " << Error << std::endl;
            }
            bAllPassed = false;
            continue;
        }

        const std::vector<std::string> ExpectationIssues = ValidateExpectations(Test.Expectation, Thread);
        if (!ExpectationIssues.empty())
        {
            std::cout << "  Expectation mismatch:" << std::endl;
            for (const std::string& Issue : ExpectationIssues)
            {
                std::cout << "    " << Issue << std::endl;
            }
            bAllPassed = false;
        }
        else
        {
            std::cout << "  Test passed." << std::endl;
        }

        PrintThreadState(Thread);
        std::cout << std::endl;
    }

    if (bAllPassed)
    {
        std::cout << "All tests passed. Tweak inline ops and cache usage to chase even fewer cycles!" << std::endl;
    }
    else
    {
        std::cout << "Some tests failed. Use the diagnostics above to adjust your strategy." << std::endl;
    }
    std::cout << std::endl;
}

void PrintMenu()
{
    std::cout << "Menu:\n"
              << "  1) Show puzzle overview\n"
              << "  2) Show current code\n"
              << "  3) Edit code\n"
              << "  4) Load code from file\n"
              << "  5) Save code to file\n"
              << "  6) Run tests\n"
              << "  7) Quit\n";
}

int ReadMenuChoice()
{
    while (true)
    {
        std::string Line = PromptLine("Select option: ");
        std::stringstream Stream(Line);
        int Choice = 0;
        if (Stream >> Choice)
        {
            return Choice;
        }
        std::cout << "Invalid selection. Please enter a number from the menu." << std::endl;
    }
}
}

int main(int Argc, char* Argv[])
{
    if (Argc < 2)
    {
        std::cout << "Usage: conch_ide <puzzle.json> [optional_code_file]\n";
        return 1;
    }

    const std::string PuzzlePath = Argv[1];
    PuzzleData Puzzle;
    std::string Error;
    if (!LoadPuzzleFromFile(PuzzlePath, Puzzle, Error))
    {
        std::cout << "Failed to load puzzle: " << Error << std::endl;
        return 1;
    }

    std::vector<std::string> Code = Puzzle.StarterCode;
    if (Argc >= 3)
    {
        if (!LoadCodeFromFile(Argv[2], Code, Error))
        {
            std::cout << "Warning: " << Error << "\nUsing starter code instead." << std::endl;
            Code = Puzzle.StarterCode;
        }
    }

    std::cout << "Loaded puzzle: " << Puzzle.Title << "\n";
    ShowPuzzleOverview(Puzzle);
    ShowCode(Code);

    bool bRunning = true;
    while (bRunning && std::cin)
    {
        PrintMenu();
        const int Choice = ReadMenuChoice();
        std::cout << std::endl;
        switch (Choice)
        {
        case 1:
            ShowPuzzleOverview(Puzzle);
            break;
        case 2:
            ShowCode(Code);
            break;
        case 3:
            EditCode(Code);
            break;
        case 4:
        {
            const std::string Path = PromptLine("File to load: ");
            if (!LoadCodeFromFile(Path, Code, Error))
            {
                std::cout << Error << std::endl;
            }
            else
            {
                std::cout << "Code loaded from " << Path << "\n";
            }
            break;
        }
        case 5:
        {
            const std::string Path = PromptLine("File to save to: ");
            if (!SaveCodeToFile(Path, Code, Error))
            {
                std::cout << Error << std::endl;
            }
            else
            {
                std::cout << "Saved code to " << Path << "\n";
            }
            break;
        }
        case 6:
            RunTests(Puzzle, Code);
            break;
        case 7:
            bRunning = false;
            break;
        default:
            std::cout << "Unknown option." << std::endl;
            break;
        }
    }

    std::cout << "Good luck reducing those cycles!" << std::endl;
    return 0;
}


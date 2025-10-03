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
const char* const COLOR_RESET = "\033[0m";
const char* const COLOR_HEADING = "\033[1;36m";
const char* const COLOR_MENU = "\033[1;33m";
const char* const COLOR_SUCCESS = "\033[1;32m";
const char* const COLOR_ERROR = "\033[1;31m";
const char* const COLOR_INFO = "\033[0;36m";

bool EqualsIgnoreCase(const std::string& A, const std::string& B)
{
    if (A.size() != B.size())
    {
        return false;
    }
    for (size_t i = 0; i < A.size(); ++i)
    {
        const unsigned char Left = static_cast<unsigned char>(A[i]);
        const unsigned char Right = static_cast<unsigned char>(B[i]);
        if (std::tolower(Left) != std::tolower(Right))
        {
            return false;
        }
    }
    return true;
}

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
    std::cout << COLOR_HEADING << "=== " << Puzzle.Title << " ===" << COLOR_RESET << "\n";
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
        if (!Test.DatInputs.empty())
        {
            std::cout << "     DAT:";
            for (const PuzzleListSpec& Spec : Test.DatInputs)
            {
                std::cout << " " << Spec.Name << "=" << FormatListValues(Spec.Values);
            }
            std::cout << std::endl;
        }
        if (!Test.OutSpecs.empty())
        {
            std::cout << "     OUT:";
            for (const PuzzleOutSpec& Spec : Test.OutSpecs)
            {
                std::cout << " " << Spec.Name << "(size=" << Spec.ExpectedSize << ")";
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
            for (const PuzzleListSpec& Spec : Test.Expectation.ExpectedOut)
            {
                std::cout << " " << Spec.Name << "=" << FormatListValues(Spec.Values);
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
    std::cout << COLOR_HEADING << "Current program" << COLOR_RESET << " (" << Code.size() << " line" << (Code.size() == 1 ? "" : "s") << "):\n";
    for (size_t i = 0; i < Code.size(); ++i)
    {
        std::cout << COLOR_MENU << std::setw(3) << (i + 1) << COLOR_RESET << " | " << Code[i] << std::endl;
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
    std::cout << COLOR_INFO << "Enter new program lines. Type '.exit' on a blank line to finish." << COLOR_RESET << std::endl;
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
        if (EqualsIgnoreCase(Trimmed, ".exit"))
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
    for (const PuzzleListSpec& Spec : Test.DatInputs)
    {
        ConVariableList* List = Thread.FindListVar(Spec.Name);
        if (List == nullptr)
        {
            Messages.push_back(Spec.Name + " is not defined in this program");
            bSuccess = false;
            continue;
        }
        std::vector<int32> Values;
        Values.reserve(Spec.Values.size());
        for (int Value : Spec.Values)
        {
            Values.push_back(static_cast<int32>(Value));
        }
        List->SetRole(ConListRole::Input);
        List->SetExpectedSize(std::numeric_limits<size_t>::max());
        List->SetValues(Values);
        List->Reset();
    }
    for (const PuzzleOutSpec& Spec : Test.OutSpecs)
    {
        ConVariableList* List = Thread.FindListVar(Spec.Name);
        if (List == nullptr)
        {
            Messages.push_back(Spec.Name + " is not defined in this program");
            bSuccess = false;
            continue;
        }
        List->SetRole(ConListRole::Output);
        List->SetValues(std::vector<int32>());
        List->SetExpectedSize(static_cast<size_t>(Spec.ExpectedSize));
        List->Reset();
    }
    return bSuccess;
}

std::vector<std::string> ValidateExpectations(const PuzzleTestCase& Test, const ConThread& Thread)
{
    const PuzzleExpectation& Expectation = Test.Expectation;
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
    for (const PuzzleOutSpec& Spec : Test.OutSpecs)
    {
        const ConVariableList* List = Thread.FindListVar(Spec.Name);
        if (List == nullptr)
        {
            Issues.push_back("Expectation references undefined " + Spec.Name);
            continue;
        }
        const size_t ActualSize = List->Size();
        const size_t ExpectedSize = static_cast<size_t>(Spec.ExpectedSize);
        if (ActualSize != ExpectedSize)
        {
            Issues.push_back("Expected " + Spec.Name + " size=" + std::to_string(ExpectedSize) + ", got " + std::to_string(ActualSize));
        }
    }
    for (const PuzzleListSpec& Spec : Expectation.ExpectedOut)
    {
        const ConVariableList* List = Thread.FindListVar(Spec.Name);
        if (List == nullptr)
        {
            Issues.push_back("Expectation references undefined " + Spec.Name);
            continue;
        }
        const std::vector<int32>& Actual = List->GetValues();
        const std::vector<int> ActualCopy(Actual.begin(), Actual.end());
        if (ActualCopy.size() != Spec.Values.size() || !std::equal(ActualCopy.begin(), ActualCopy.end(), Spec.Values.begin()))
        {
            Issues.push_back("Expected " + Spec.Name + "=" + FormatListValues(Spec.Values) + ", got " + FormatListValues(ActualCopy));
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
    std::cout << "    " << COLOR_INFO << "Registers:" << COLOR_RESET;
    for (size_t i = 0; i < Thread.GetThreadVarCount(); ++i)
    {
        std::cout << " " << RegisterName(i) << "=" << Thread.GetThreadValue(i) << " (C=" << Thread.GetThreadCacheValue(i) << ")";
    }
    std::cout << std::endl;
    const std::vector<std::string> ListNames = Thread.GetListNames();
    if (!ListNames.empty())
    {
        std::cout << "    " << COLOR_INFO << "Lists:" << COLOR_RESET;
        for (const std::string& Name : ListNames)
        {
            const ConVariableList* List = Thread.FindListVar(Name);
            if (List != nullptr)
            {
                const std::vector<int32>& Values = List->GetValues();
                std::vector<int> Copy(Values.begin(), Values.end());
                std::cout << " " << Name << "=" << FormatListValues(Copy);
            }
        }
        std::cout << std::endl;
    }
}

void RunTests(const PuzzleData& Puzzle, const std::vector<std::string>& Code, bool bDebugTrace)
{
    if (Code.empty())
    {
        std::cout << COLOR_ERROR << "No code to execute. Use the editor or load a file first.\n" << COLOR_RESET;
        return;
    }

    std::vector<std::string> ParseErrors;
    int StaticCycles = 0;
    if (!ComputeStaticCycleCount(Code, StaticCycles, ParseErrors))
    {
        std::cout << COLOR_ERROR << "Syntax errors detected:" << COLOR_RESET << "\n";
        for (const std::string& Error : ParseErrors)
        {
            std::cout << COLOR_ERROR << "  " << Error << COLOR_RESET << std::endl;
        }
        return;
    }

    std::cout << COLOR_INFO << "Static cycle estimate: " << StaticCycles << COLOR_RESET << std::endl;
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
                std::cout << COLOR_SUCCESS << "You are on track to beat the recorded best of " << Best->Cycles << " cycles (" << Best->Label << ")!" << COLOR_RESET << std::endl;
            }
            else if (StaticCycles == Best->Cycles)
            {
                std::cout << COLOR_INFO << "Matched the best recorded cycle count of " << Best->Cycles << " (" << Best->Label << ")." << COLOR_RESET << std::endl;
            }
            else
            {
                std::cout << COLOR_INFO << "Need " << (StaticCycles - Best->Cycles) << " fewer cycles to beat the best record (" << Best->Label << ")." << COLOR_RESET << std::endl;
            }
        }
    }
    std::cout << std::endl;

    if (bDebugTrace)
    {
        std::cout << COLOR_INFO << "Debug trace enabled: register states will be printed after each executed line." << COLOR_RESET << std::endl;
    }

    bool bAllPassed = true;

    for (size_t TestIndex = 0; TestIndex < Puzzle.Tests.size(); ++TestIndex)
    {
        const PuzzleTestCase& Test = Puzzle.Tests[TestIndex];
        std::cout << COLOR_HEADING << "Running test " << (TestIndex + 1) << "/" << Puzzle.Tests.size() << ": " << Test.Name << COLOR_RESET << std::endl;

        ConParser Parser;
        ConThread Thread;
        if (!Parser.Parse(Code, Thread))
        {
            std::cout << COLOR_ERROR << "  Parse failed during execution." << COLOR_RESET << "\n";
            for (const std::string& Error : Parser.GetErrors())
            {
                std::cout << COLOR_ERROR << "    " << Error << COLOR_RESET << std::endl;
            }
            bAllPassed = false;
            continue;
        }

        Thread.SetTraceEnabled(bDebugTrace);
        Thread.UpdateCycleCount();

        std::vector<std::string> SetupMessages;
        if (!ApplyTestSetup(Test, Thread, SetupMessages))
        {
            std::cout << COLOR_ERROR << "  Test setup failed:" << COLOR_RESET << std::endl;
            for (const std::string& Message : SetupMessages)
            {
                std::cout << COLOR_ERROR << "    " << Message << COLOR_RESET << std::endl;
            }
            bAllPassed = false;
            continue;
        }

        Thread.Execute();
        if (Thread.HadRuntimeError())
        {
            std::cout << COLOR_ERROR << "  Runtime error:" << COLOR_RESET << std::endl;
            for (const std::string& Error : Thread.GetRuntimeErrors())
            {
                std::cout << COLOR_ERROR << "    " << Error << COLOR_RESET << std::endl;
            }
            bAllPassed = false;
            continue;
        }

        const std::vector<std::string> ExpectationIssues = ValidateExpectations(Test, Thread);
        if (!ExpectationIssues.empty())
        {
            std::cout << COLOR_ERROR << "  Expectation mismatch:" << COLOR_RESET << std::endl;
            for (const std::string& Issue : ExpectationIssues)
            {
                std::cout << COLOR_ERROR << "    " << Issue << COLOR_RESET << std::endl;
            }
            bAllPassed = false;
        }
        else
        {
            std::cout << COLOR_SUCCESS << "  Test passed." << COLOR_RESET << std::endl;
        }

        PrintThreadState(Thread);
        std::cout << std::endl;
    }

    if (bAllPassed)
    {
        std::cout << COLOR_SUCCESS << "All tests passed. Tweak inline ops and cache usage to chase even fewer cycles!" << COLOR_RESET << std::endl;
    }
    else
    {
        std::cout << COLOR_ERROR << "Some tests failed. Use the diagnostics above to adjust your strategy." << COLOR_RESET << std::endl;
    }
    std::cout << std::endl;
}

void PrintMenu(bool bDebugTraceEnabled)
{
    std::cout << COLOR_MENU << "Menu:\n"
              << "  1) Show puzzle overview\n"
              << "  2) Show current code\n"
              << "  3) Edit code\n"
              << "  4) Load code from file\n"
              << "  5) Save code to file\n"
              << "  6) Run tests\n"
              << "  7) Toggle debug trace (" << (bDebugTraceEnabled ? "ON" : "OFF") << ")\n"
              << "  8) Quit\n"
              << COLOR_RESET;
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
        std::cout << COLOR_ERROR << "Invalid selection. Please enter a number from the menu." << COLOR_RESET << std::endl;
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

    std::cout << COLOR_HEADING << "Loaded puzzle: " << Puzzle.Title << COLOR_RESET << "\n";
    ShowPuzzleOverview(Puzzle);
    ShowCode(Code);

    bool bRunning = true;
    bool bDebugTraceEnabled = false;
    while (bRunning && std::cin)
    {
        PrintMenu(bDebugTraceEnabled);
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
            RunTests(Puzzle, Code, bDebugTraceEnabled);
            break;
        case 7:
            bDebugTraceEnabled = !bDebugTraceEnabled;
            std::cout << COLOR_INFO << "Debug trace " << (bDebugTraceEnabled ? "enabled" : "disabled") << "." << COLOR_RESET << std::endl;
            break;
        case 8:
            bRunning = false;
            break;
        default:
            std::cout << COLOR_ERROR << "Unknown option." << COLOR_RESET << std::endl;
            break;
        }
    }

    std::cout << COLOR_SUCCESS << "Good luck reducing those cycles!" << COLOR_RESET << std::endl;
    return 0;
}


#include "Puzzle.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Platform-specific terminal support
#ifdef _WIN32
#  define PLATFORM_WINDOWS 1
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <conio.h>
#  include <windows.h>
#  undef min
#  undef max
#  undef Type
#  undef constant
#else
#  define PLATFORM_WINDOWS 0
#  include <sys/ioctl.h>
#  include <termios.h>
#  include <unistd.h>
#endif

#include "../src/Conchpiler/parser.h"
#include "../src/Conchpiler/thread.h"

namespace
{

// ============================================================
// ANSI escape helpers
// ============================================================

const char* const COLOR_RESET   = "\033[0m";
const char* const COLOR_HEADING = "\033[1;36m";
const char* const COLOR_SUCCESS = "\033[1;32m";
const char* const COLOR_ERROR   = "\033[1;31m";
const char* const COLOR_INFO    = "\033[0;36m";
const char* const COLOR_GRAY    = "\033[0;37m";
const char* const COLOR_OPTION  = "\033[1;33m";
const char* const ANSI_HIDE_CURSOR = "\033[?25l";
const char* const ANSI_SHOW_CURSOR = "\033[?25h";
const char* const ANSI_CLEAR_SCREEN = "\033[2J";

inline std::string AnsiMoveTo(int Row, int Col)
{
    return "\033[" + std::to_string(Row) + ";" + std::to_string(Col) + "H";
}

inline std::string AnsiSetBg(int Code) { return "\033[" + std::to_string(Code) + "m"; }
const char* const ANSI_REVERSE   = "\033[7m";  // reverse-video (status bars)
const char* const ANSI_BOLD      = "\033[1m";
const char* const ANSI_CLEAR_EOL = "\033[K";   // clear to end of line

// ============================================================
// Misc string utilities (used throughout)
// ============================================================

std::string RegisterName(size_t Index)
{
    static const char* Names[] = {"X", "Y", "Z"};
    if (Index < 3) return Names[Index];
    return "R" + std::to_string(Index);
}

std::string FormatListValues(const std::vector<int>& Values)
{
    std::ostringstream Oss;
    Oss << "[";
    for (size_t i = 0; i < Values.size(); ++i)
    {
        if (i) Oss << ", ";
        Oss << Values[i];
    }
    Oss << "]";
    return Oss.str();
}

std::vector<std::pair<std::string,int>>
SortRegisterMap(const std::unordered_map<std::string,int>& Registers)
{
    std::vector<std::pair<std::string,int>> Sorted(Registers.begin(), Registers.end());
    std::sort(Sorted.begin(), Sorted.end(),
        [](const auto& A, const auto& B){ return A.first < B.first; });
    return Sorted;
}

// ============================================================
// Puzzle file helpers
// ============================================================

std::vector<std::filesystem::path> FindPuzzleFiles(const std::filesystem::path& Dir)
{
    std::vector<std::filesystem::path> Files;
    std::error_code EC;
    for (const auto& Entry : std::filesystem::directory_iterator(Dir, EC))
    {
        std::error_code EntryEC;
        if (Entry.is_regular_file(EntryEC) && !EntryEC &&
            Entry.path().extension() == ".json")
            Files.push_back(Entry.path());
    }
    std::sort(Files.begin(), Files.end());
    return Files;
}

std::filesystem::path FindPuzzlesDirectory(const std::filesystem::path& ExecutablePath)
{
    std::vector<std::filesystem::path> Candidates;
    std::unordered_set<std::string> SeenCandidates;
    const std::vector<std::filesystem::path> SearchRoots = {
        ExecutablePath.parent_path(),
        std::filesystem::current_path(),
    };

    const auto AddCandidate = [&](const std::filesystem::path& Candidate)
    {
        const std::string Key = Candidate.lexically_normal().generic_string();
        if (SeenCandidates.insert(Key).second)
            Candidates.push_back(Candidate);
    };

    for (const auto& Root : SearchRoots)
    {
        for (std::filesystem::path Probe = Root; !Probe.empty(); Probe = Probe.parent_path())
        {
            AddCandidate(Probe / "Puzzles");
            AddCandidate(Probe / "TestApp" / "Puzzles");
            if (Probe == Probe.parent_path()) break;
        }
    }

    for (const auto& C : Candidates)
    {
        std::error_code EC;
        if (std::filesystem::is_directory(C, EC)) return C;
    }
    return {};
}

std::string ReadPuzzleTitle(const std::filesystem::path& FilePath)
{
    std::ifstream Input(FilePath);
    if (!Input) return FilePath.stem().string();
    std::ostringstream Buffer;
    Buffer << Input.rdbuf();
    SimpleJsonValue Root;
    std::string Err;
    if (!SimpleJsonValue::Parse(Buffer.str(), Root, Err) || !Root.IsObject())
        return FilePath.stem().string();
    const SimpleJsonValue* Title = Root.Find("title");
    if (Title && Title->IsString()) return Title->AsString();
    return FilePath.stem().string();
}

std::filesystem::path SelectPuzzleInteractively(const std::filesystem::path& PuzzlesDir)
{
    const std::vector<std::filesystem::path> Files = FindPuzzleFiles(PuzzlesDir);
    if (Files.empty())
    {
        std::cout << COLOR_ERROR << "No puzzle files found in: "
                  << PuzzlesDir.string() << COLOR_RESET << "\n";
        return {};
    }

    std::cout << COLOR_GRAY << std::string(60, '-') << COLOR_RESET << "\n";
    std::cout << COLOR_HEADING << "Available Puzzles" << COLOR_RESET << "\n";
    std::cout << COLOR_GRAY << std::string(60, '-') << COLOR_RESET << "\n";
    for (size_t i = 0; i < Files.size(); ++i)
    {
        std::cout << COLOR_OPTION << "  " << std::setw(2) << (i + 1) << COLOR_RESET
                  << ". " << ReadPuzzleTitle(Files[i])
                  << COLOR_GRAY << "  (" << Files[i].filename().string() << ")"
                  << COLOR_RESET << "\n";
    }
    std::cout << COLOR_OPTION << "   0" << COLOR_RESET << ". Cancel\n";
    std::cout << COLOR_GRAY << std::string(60, '-') << COLOR_RESET << "\n";

    while (true)
    {
        std::cout << "Select a puzzle: ";
        std::string Line;
        std::getline(std::cin, Line);
        if (!std::cin) return {};
        std::stringstream Stream(Line);
        int Choice = -1;
        if (Stream >> Choice)
        {
            if (Choice == 0) return {};
            if (Choice > 0 && static_cast<size_t>(Choice) <= Files.size())
                return Files[static_cast<size_t>(Choice) - 1];
        }
        std::cout << COLOR_ERROR << "Please enter a number between 0 and "
                  << Files.size() << "." << COLOR_RESET << "\n";
    }
}

bool LoadCodeFromFile(const std::string& Path,
                      std::vector<std::string>& OutCode,
                      std::string& OutError)
{
    std::ifstream Input(Path);
    if (!Input) { OutError = "Unable to open file: " + Path; return false; }
    std::vector<std::string> Lines;
    std::string Line;
    while (std::getline(Input, Line)) Lines.push_back(Line);
    OutCode = std::move(Lines);
    return true;
}

bool SaveCodeToFile(const std::string& Path,
                    const std::vector<std::string>& Code,
                    std::string& OutError)
{
    std::ofstream Output(Path);
    if (!Output) { OutError = "Unable to write to file: " + Path; return false; }
    for (size_t i = 0; i < Code.size(); ++i)
    {
        Output << Code[i];
        if (i + 1 < Code.size()) Output << '\n';
    }
    return true;
}

// ============================================================
// Test-runner helpers
// ============================================================

bool ApplyTestSetup(const PuzzleTestCase& Test,
                    ConThread& Thread,
                    std::vector<std::string>& Messages)
{
    bool bSuccess = true;
    for (const auto& Pair : Test.InitialRegisters)
    {
        int Idx = 0;
        if (!ParseRegisterName(Pair.first, Idx))
        {
            Messages.push_back("Unknown register '" + Pair.first + "'");
            bSuccess = false;
            continue;
        }
        Thread.SetThreadValue(static_cast<size_t>(Idx), Pair.second);
    }
    for (const PuzzleListSpec& Spec : Test.DatInputs)
    {
        ConVariableList* List = Thread.FindListVar(Spec.Name);
        if (!List)
        {
            Messages.push_back(Spec.Name + " is not defined in this program");
            bSuccess = false;
            continue;
        }
        std::vector<int32> Values;
        Values.reserve(Spec.Values.size());
        for (int V : Spec.Values) Values.push_back(static_cast<int32>(V));
        List->SetRole(ConListRole::Input);
        List->SetExpectedSize(std::numeric_limits<size_t>::max());
        List->SetValues(Values);
        List->Reset();
    }
    for (const PuzzleOutSpec& Spec : Test.OutSpecs)
    {
        ConVariableList* List = Thread.FindListVar(Spec.Name);
        if (!List)
        {
            Messages.push_back(Spec.Name + " is not defined in this program");
            bSuccess = false;
            continue;
        }
        List->SetRole(ConListRole::Output);
        List->SetValues({});
        List->SetExpectedSize(static_cast<size_t>(Spec.ExpectedSize));
        List->Reset();
    }
    return bSuccess;
}

std::vector<std::string> ValidateExpectations(const PuzzleTestCase& Test,
                                              const ConThread& Thread)
{
    std::vector<std::string> Issues;
    for (const auto& Pair : Test.Expectation.Registers)
    {
        int Idx = 0;
        if (!ParseRegisterName(Pair.first, Idx))
        {
            Issues.push_back("Unknown register '" + Pair.first + "'");
            continue;
        }
        const int Actual = Thread.GetThreadValue(static_cast<size_t>(Idx));
        if (Actual != Pair.second)
            Issues.push_back("Expected " + Pair.first + "=" +
                             std::to_string(Pair.second) + ", got " +
                             std::to_string(Actual));
    }
    for (const PuzzleOutSpec& Spec : Test.OutSpecs)
    {
        const ConVariableList* List = Thread.FindListVar(Spec.Name);
        if (!List) { Issues.push_back("Undefined " + Spec.Name); continue; }
        const size_t ActualSize  = List->Size();
        const size_t ExpectedSize = static_cast<size_t>(Spec.ExpectedSize);
        if (ActualSize != ExpectedSize)
            Issues.push_back("Expected " + Spec.Name + " size=" +
                             std::to_string(ExpectedSize) + ", got " +
                             std::to_string(ActualSize));
    }
    for (const PuzzleListSpec& Spec : Test.Expectation.ExpectedOut)
    {
        const ConVariableList* List = Thread.FindListVar(Spec.Name);
        if (!List) { Issues.push_back("Undefined " + Spec.Name); continue; }
        const std::vector<int32>& Actual = List->GetValues();
        const std::vector<int> Copy(Actual.begin(), Actual.end());
        if (Copy.size() != Spec.Values.size() ||
            !std::equal(Copy.begin(), Copy.end(), Spec.Values.begin()))
            Issues.push_back("Expected " + Spec.Name + "=" +
                             FormatListValues(Spec.Values) + ", got " +
                             FormatListValues(Copy));
    }
    return Issues;
}

bool ComputeStaticCycleCount(const std::vector<std::string>& Code,
                             int& OutCycles,
                             std::vector<std::string>& Errors)
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

// Runs all tests and returns a vector of output lines (no ANSI codes).
// Each entry is one display line.
std::vector<std::string> CollectTestOutput(const PuzzleData& Puzzle,
                                           const std::vector<std::string>& Code,
                                           bool bDebugTrace)
{
    std::vector<std::string> Out;

    if (Code.empty())
    {
        Out.push_back("ERROR: No code to run. Write some code first.");
        return Out;
    }

    std::vector<std::string> ParseErrors;
    int StaticCycles = 0;
    if (!ComputeStaticCycleCount(Code, StaticCycles, ParseErrors))
    {
        Out.push_back("Syntax errors detected:");
        for (const std::string& E : ParseErrors)
            Out.push_back("  " + E);
        return Out;
    }
    Out.push_back("Static cycle estimate: " + std::to_string(StaticCycles));

    if (!Puzzle.History.empty())
    {
        const auto Best = std::min_element(Puzzle.History.begin(), Puzzle.History.end(),
            [](const PuzzleHistoryEntry& A, const PuzzleHistoryEntry& B){
                return A.Cycles < B.Cycles;
            });
        if (Best != Puzzle.History.end())
        {
            if (StaticCycles < Best->Cycles)
                Out.push_back("On track to beat best (" + Best->Label + ", " +
                              std::to_string(Best->Cycles) + " cycles)!");
            else if (StaticCycles == Best->Cycles)
                Out.push_back("Matched best record (" + Best->Label + ", " +
                              std::to_string(Best->Cycles) + " cycles).");
            else
                Out.push_back("Need " + std::to_string(StaticCycles - Best->Cycles) +
                              " fewer cycles to beat " + Best->Label + " (" +
                              std::to_string(Best->Cycles) + ").");
        }
    }
    Out.push_back("");

    bool bAllPassed = true;
    for (size_t TestIndex = 0; TestIndex < Puzzle.Tests.size(); ++TestIndex)
    {
        const PuzzleTestCase& Test = Puzzle.Tests[TestIndex];
        Out.push_back("Test " + std::to_string(TestIndex + 1) + "/" +
                      std::to_string(Puzzle.Tests.size()) + ": " + Test.Name);

        ConParser Parser;
        ConThread Thread;
        if (!Parser.Parse(Code, Thread))
        {
            Out.push_back("  FAIL: Parse error during execution.");
            for (const std::string& E : Parser.GetErrors())
                Out.push_back("    " + E);
            bAllPassed = false;
            continue;
        }
        Thread.SetTraceEnabled(bDebugTrace);
        Thread.UpdateCycleCount();

        std::vector<std::string> SetupMsgs;
        if (!ApplyTestSetup(Test, Thread, SetupMsgs))
        {
            Out.push_back("  FAIL: Test setup failed:");
            for (const std::string& M : SetupMsgs) Out.push_back("    " + M);
            bAllPassed = false;
            continue;
        }

        Thread.Execute();
        if (Thread.HadRuntimeError())
        {
            Out.push_back("  FAIL: Runtime error:");
            for (const std::string& E : Thread.GetRuntimeErrors())
                Out.push_back("    " + E);
            bAllPassed = false;
            continue;
        }

        const auto Issues = ValidateExpectations(Test, Thread);
        if (!Issues.empty())
        {
            Out.push_back("  FAIL: Expectation mismatch:");
            for (const std::string& I : Issues) Out.push_back("    " + I);
            bAllPassed = false;
        }
        else
        {
            Out.push_back("  PASS");
        }

        // Register states
        std::string RegLine = "  Regs:";
        for (size_t i = 0; i < Thread.GetThreadVarCount(); ++i)
            RegLine += " " + RegisterName(i) + "=" +
                       std::to_string(Thread.GetThreadValue(i));
        Out.push_back(RegLine);

        const std::vector<std::string> ListNames = Thread.GetListNames();
        if (!ListNames.empty())
        {
            std::string ListLine = "  Lists:";
            for (const std::string& N : ListNames)
            {
                const ConVariableList* L = Thread.FindListVar(N);
                if (L)
                {
                    const std::vector<int32>& V = L->GetValues();
                    std::vector<int> C(V.begin(), V.end());
                    ListLine += " " + N + "=" + FormatListValues(C);
                }
            }
            Out.push_back(ListLine);
        }
        Out.push_back("");
    }

    Out.push_back(bAllPassed
        ? "All tests PASSED!  Tweak inline ops to chase fewer cycles."
        : "Some tests FAILED. See details above.");
    return Out;
}

// Build a vector of lines describing the puzzle overview (no ANSI).
std::vector<std::string> CollectPuzzleOverview(const PuzzleData& Puzzle)
{
    std::vector<std::string> Out;
    Out.push_back("=== " + Puzzle.Title + " ===");
    Out.push_back("");
    if (!Puzzle.Description.empty())
    {
        // Word-wrap description at ~72 chars
        std::istringstream Desc(Puzzle.Description);
        std::string Word, CurrentLine;
        while (Desc >> Word)
        {
            if (!CurrentLine.empty() && CurrentLine.size() + 1 + Word.size() > 72)
            {
                Out.push_back(CurrentLine);
                CurrentLine = Word;
            }
            else
            {
                if (!CurrentLine.empty()) CurrentLine += ' ';
                CurrentLine += Word;
            }
        }
        if (!CurrentLine.empty()) Out.push_back(CurrentLine);
        Out.push_back("");
    }
    Out.push_back("Tests:");
    for (size_t i = 0; i < Puzzle.Tests.size(); ++i)
    {
        const PuzzleTestCase& T = Puzzle.Tests[i];
        Out.push_back("  " + std::to_string(i + 1) + ". " + T.Name);
        if (!T.InitialRegisters.empty())
        {
            std::string Line = "     Registers:";
            for (const auto& P : SortRegisterMap(T.InitialRegisters))
                Line += " " + P.first + "=" + std::to_string(P.second);
            Out.push_back(Line);
        }
        if (!T.DatInputs.empty())
        {
            std::string Line = "     DAT:";
            for (const PuzzleListSpec& S : T.DatInputs)
                Line += " " + S.Name + "=" + FormatListValues(S.Values);
            Out.push_back(Line);
        }
        if (!T.OutSpecs.empty())
        {
            std::string Line = "     OUT:";
            for (const PuzzleOutSpec& S : T.OutSpecs)
                Line += " " + S.Name + "(size=" + std::to_string(S.ExpectedSize) + ")";
            Out.push_back(Line);
        }
        if (T.Expectation.HasExpectations())
        {
            std::string Line = "     Expect:";
            for (const auto& P : SortRegisterMap(T.Expectation.Registers))
                Line += " " + P.first + "=" + std::to_string(P.second);
            for (const PuzzleListSpec& S : T.Expectation.ExpectedOut)
                Line += " " + S.Name + "=" + FormatListValues(S.Values);
            Out.push_back(Line);
        }
    }
    if (!Puzzle.History.empty())
    {
        Out.push_back("");
        Out.push_back("Cycle history:");
        for (const PuzzleHistoryEntry& E : Puzzle.History)
            Out.push_back("  " + std::to_string(E.Cycles) + " cycles - " + E.Label);
    }
    return Out;
}

std::vector<std::string> BuildHelpLines()
{
    return {
        "=== Conch IDE Key Reference ===",
        "",
        "Navigation:",
        "  Arrow keys       Move cursor",
        "  Home / End       Jump to start / end of line",
        "  PgUp / PgDn      Scroll page up / down",
        "",
        "Editing:",
        "  Type              Insert character",
        "  Enter             Insert new line",
        "  Backspace         Delete char to the left",
        "  Del               Delete char under cursor",
        "  Tab               Insert 2 spaces",
        "  Ctrl+K            Delete current line",
        "  Ctrl+U            Undo last line deletion",
        "",
        "File:",
        "  Ctrl+S            Save code to file",
        "  Ctrl+O            Open / load code from file",
        "",
        "Actions:",
        "  Ctrl+R or F5      Run tests",
        "  Ctrl+P            Show puzzle overview",
        "  F1                Show this help",
        "  Ctrl+D            Toggle debug trace",
        "",
        "Exit:",
        "  Ctrl+Q or Esc     Quit (prompts if unsaved)",
        "",
        "Press any key to close this panel.",
    };
}

// ============================================================
// Platform terminal: raw mode, terminal size, key reading
// ============================================================

#if PLATFORM_WINDOWS
struct TerminalState
{
    DWORD OrigConsoleMode = 0;
    HANDLE StdinHandle  = INVALID_HANDLE_VALUE;
    bool   bRawMode     = false;
};
#else
struct TerminalState
{
    struct termios Orig;
    bool bRawMode = false;
};
#endif

TerminalState& GetTerminalState()
{
    static TerminalState State;
    return State;
}

bool EnableRawMode()
{
    TerminalState& S = GetTerminalState();
#if PLATFORM_WINDOWS
    S.StdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    if (S.StdinHandle == INVALID_HANDLE_VALUE) return false;
    if (!GetConsoleMode(S.StdinHandle, &S.OrigConsoleMode)) return false;
    DWORD New = S.OrigConsoleMode &
        ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    if (!SetConsoleMode(S.StdinHandle, New)) return false;
    // Enable ANSI on Windows 10+
    HANDLE Stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD OutMode = 0;
    GetConsoleMode(Stdout, &OutMode);
    SetConsoleMode(Stdout, OutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    S.bRawMode = true;
    return true;
#else
    if (!isatty(STDIN_FILENO)) return false;
    if (tcgetattr(STDIN_FILENO, &S.Orig) == -1) return false;
    struct termios Raw = S.Orig;
    Raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    Raw.c_oflag &= ~(OPOST);
    Raw.c_cflag |=  (CS8);
    Raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    Raw.c_cc[VMIN]  = 1;
    Raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &Raw) == -1) return false;
    S.bRawMode = true;
    return true;
#endif
}

void DisableRawMode()
{
    TerminalState& S = GetTerminalState();
    if (!S.bRawMode) return;
#if PLATFORM_WINDOWS
    if (S.StdinHandle != INVALID_HANDLE_VALUE)
        SetConsoleMode(S.StdinHandle, S.OrigConsoleMode);
#else
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &S.Orig);
#endif
    S.bRawMode = false;
}

bool GetTerminalSize(int& OutRows, int& OutCols)
{
#if PLATFORM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO Info;
    HANDLE H = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(H, &Info))
    {
        OutRows = Info.srWindow.Bottom - Info.srWindow.Top + 1;
        OutCols = Info.srWindow.Right  - Info.srWindow.Left + 1;
        return true;
    }
#else
    struct winsize WS;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &WS) == 0 && WS.ws_col > 0)
    {
        OutRows = WS.ws_row;
        OutCols = WS.ws_col;
        return true;
    }
#endif
    OutRows = 24;
    OutCols = 80;
    return false;
}

// ============================================================
// Keyboard input
// ============================================================

enum class EditorKey
{
    Regular,
    ArrowUp, ArrowDown, ArrowLeft, ArrowRight,
    Home, End,
    PageUp, PageDown,
    DeleteKey,
    Backspace,
    Enter,
    Tab,
    Escape,
    CtrlS, CtrlO, CtrlR, CtrlQ, CtrlK, CtrlU, CtrlP, CtrlD,
    F1, F5,
    Unknown,
};

struct KeyInput
{
    EditorKey Key = EditorKey::Unknown;
    char Ch = 0;
};

KeyInput ReadKey()
{
    KeyInput Result;

#if PLATFORM_WINDOWS
    int Code = _getch();
    if (Code == 0 || Code == 0xE0)
    {
        const int Code2 = _getch();
        switch (Code2)
        {
        case 72: Result.Key = EditorKey::ArrowUp;    return Result;
        case 80: Result.Key = EditorKey::ArrowDown;  return Result;
        case 75: Result.Key = EditorKey::ArrowLeft;  return Result;
        case 77: Result.Key = EditorKey::ArrowRight; return Result;
        case 71: Result.Key = EditorKey::Home;        return Result;
        case 79: Result.Key = EditorKey::End;         return Result;
        case 73: Result.Key = EditorKey::PageUp;      return Result;
        case 81: Result.Key = EditorKey::PageDown;    return Result;
        case 83: Result.Key = EditorKey::DeleteKey;   return Result;
        case 59: Result.Key = EditorKey::F1;          return Result;
        case 63: Result.Key = EditorKey::F5;          return Result;
        default: Result.Key = EditorKey::Unknown;     return Result;
        }
    }
    char C = static_cast<char>(Code);
#else
    char C;
    if (read(STDIN_FILENO, &C, 1) != 1)
    {
        Result.Key = EditorKey::Unknown;
        return Result;
    }
#endif

    // Escape or escape sequence
    if (C == '\033')
    {
#if !PLATFORM_WINDOWS
        char Seq[6] = {};
        // Short read timeout: if nothing follows, it is a bare Escape key
        if (read(STDIN_FILENO, &Seq[0], 1) != 1)
        {
            Result.Key = EditorKey::Escape;
            return Result;
        }
        if (read(STDIN_FILENO, &Seq[1], 1) != 1)
        {
            Result.Key = EditorKey::Escape;
            return Result;
        }

        if (Seq[0] == '[')
        {
            if (Seq[1] >= '0' && Seq[1] <= '9')
            {
                // Extended: ESC [ N ~  or  ESC [ 1 5 ~
                if (read(STDIN_FILENO, &Seq[2], 1) != 1)
                {
                    Result.Key = EditorKey::Unknown;
                    return Result;
                }
                if (Seq[2] == '~')
                {
                    switch (Seq[1])
                    {
                    case '1': Result.Key = EditorKey::Home;     return Result;
                    case '3': Result.Key = EditorKey::DeleteKey;return Result;
                    case '4': Result.Key = EditorKey::End;      return Result;
                    case '5': Result.Key = EditorKey::PageUp;   return Result;
                    case '6': Result.Key = EditorKey::PageDown; return Result;
                    case '7': Result.Key = EditorKey::Home;     return Result;
                    case '8': Result.Key = EditorKey::End;      return Result;
                    }
                }
                else if (Seq[2] >= '0' && Seq[2] <= '9')
                {
                    // Two-digit sequences: ESC [ 1 5 ~  etc.
                    char Tilde;
                    if (read(STDIN_FILENO, &Tilde, 1) == 1 && Tilde == '~')
                    {
                        const int Code = (Seq[1] - '0') * 10 + (Seq[2] - '0');
                        switch (Code)
                        {
                        case 11: Result.Key = EditorKey::F1; return Result;
                        case 15: Result.Key = EditorKey::F5; return Result;
                        }
                    }
                }
            }
            else
            {
                switch (Seq[1])
                {
                case 'A': Result.Key = EditorKey::ArrowUp;    return Result;
                case 'B': Result.Key = EditorKey::ArrowDown;  return Result;
                case 'C': Result.Key = EditorKey::ArrowRight; return Result;
                case 'D': Result.Key = EditorKey::ArrowLeft;  return Result;
                case 'H': Result.Key = EditorKey::Home;       return Result;
                case 'F': Result.Key = EditorKey::End;        return Result;
                }
            }
        }
        else if (Seq[0] == 'O')
        {
            switch (Seq[1])
            {
            case 'P': Result.Key = EditorKey::F1; return Result;
            case 'S': Result.Key = EditorKey::F5; return Result;
            }
        }
#endif
        Result.Key = EditorKey::Escape;
        return Result;
    }

    // Special control characters
    switch (static_cast<unsigned char>(C))
    {
    case '\r':
    case '\n':  Result.Key = EditorKey::Enter;     return Result;
    case 127:
    case   8:   Result.Key = EditorKey::Backspace; return Result;
    case '\t':  Result.Key = EditorKey::Tab;       return Result;
    case 0x13:  Result.Key = EditorKey::CtrlS;     return Result; // ^S
    case 0x0F:  Result.Key = EditorKey::CtrlO;     return Result; // ^O
    case 0x12:  Result.Key = EditorKey::CtrlR;     return Result; // ^R
    case 0x11:  Result.Key = EditorKey::CtrlQ;     return Result; // ^Q
    case 0x0B:  Result.Key = EditorKey::CtrlK;     return Result; // ^K
    case 0x15:  Result.Key = EditorKey::CtrlU;     return Result; // ^U
    case 0x10:  Result.Key = EditorKey::CtrlP;     return Result; // ^P
    case 0x04:  Result.Key = EditorKey::CtrlD;     return Result; // ^D
    }

    // Printable ASCII
    if (static_cast<unsigned char>(C) >= 32 && static_cast<unsigned char>(C) < 128)
    {
        Result.Key = EditorKey::Regular;
        Result.Ch  = C;
        return Result;
    }

    Result.Key = EditorKey::Unknown;
    return Result;
}

// ============================================================
// Editor state
// ============================================================

enum class OverlayKind { None, TestOutput, PuzzleInfo, Help };

struct EditorState
{
    std::vector<std::string> Lines;
    int CursorRow  = 0;
    int CursorCol  = 0;
    int ScrollRow  = 0;
    int ScreenRows = 24;
    int ScreenCols = 80;
    bool bModified = false;
    bool bDebugTrace = false;
    std::string Filename;
    std::string PuzzleTitle;

    // Status message (auto-expires after a few seconds)
    std::string StatusMsg;
    std::chrono::steady_clock::time_point StatusMsgTime;

    // Overlay panel
    OverlayKind OverlayType = OverlayKind::None;
    std::vector<std::string> OverlayLines;
    int OverlayScroll = 0;

    // Prompt bar (single-line input replacing status bar)
    bool bPromptActive = false;
    std::string PromptLabel;
    std::string PromptBuffer;
    std::function<void(bool, const std::string&)> PromptDone;

    // Undo buffer for Ctrl+K (simple: last deleted line)
    std::string DeletedLine;
    int DeletedLineRow = -1;

    // Set to true to exit the editor loop
    bool bQuitRequested = false;

    void SetStatus(const std::string& Msg)
    {
        StatusMsg = Msg;
        StatusMsgTime = std::chrono::steady_clock::now();
    }

    void ClampCursor()
    {
        if (Lines.empty()) Lines.push_back({});
        CursorRow = std::max(0, std::min(CursorRow,
                                        static_cast<int>(Lines.size()) - 1));
        const int LineLen = static_cast<int>(
            Lines[static_cast<size_t>(CursorRow)].size());
        CursorCol = std::max(0, std::min(CursorCol, LineLen));
    }

    void UpdateSize()
    {
        GetTerminalSize(ScreenRows, ScreenCols);
        ScreenRows = std::max(6, ScreenRows);
        ScreenCols = std::max(20, ScreenCols);
    }

    // Ensure cursor row is visible.
    void UpdateScroll()
    {
        const int EditRows = ScreenRows - 3; // title + status + 1 blank
        if (EditRows <= 0) return;
        if (CursorRow < ScrollRow)
            ScrollRow = CursorRow;
        if (CursorRow >= ScrollRow + EditRows)
            ScrollRow = CursorRow - EditRows + 1;
        ScrollRow = std::max(0, ScrollRow);
    }
};

// ============================================================
// Drawing
// ============================================================

// Truncate a string to fit in a given display width.
std::string TruncateTo(const std::string& S, int Width)
{
    if (Width <= 0) return {};
    if (static_cast<int>(S.size()) <= Width) return S;
    return S.substr(0, static_cast<size_t>(Width));
}

// Pad a string on the right to exactly Width chars.
std::string PadRight(const std::string& S, int Width)
{
    if (static_cast<int>(S.size()) >= Width) return TruncateTo(S, Width);
    return S + std::string(static_cast<size_t>(Width) - S.size(), ' ');
}

// Render the entire screen into a string buffer and flush.
void DrawEditorScreen(EditorState& E)
{
    std::string Buf;
    Buf.reserve(static_cast<size_t>(E.ScreenRows * E.ScreenCols * 2));

    Buf += ANSI_HIDE_CURSOR;
    Buf += AnsiMoveTo(1, 1);

    // ── Title bar (row 1) ─────────────────────────────────────
    {
        std::string Title = " Conch IDE";
        if (!E.PuzzleTitle.empty()) Title += " - " + E.PuzzleTitle;
        if (!E.Filename.empty())    Title += "  [" + E.Filename + "]";
        if (E.bModified)            Title += "  *Modified*";
        if (E.bDebugTrace)          Title += "  [TRACE]";
        Title = PadRight(Title, E.ScreenCols);
        Buf += ANSI_REVERSE;
        Buf += ANSI_BOLD;
        Buf += TruncateTo(Title, E.ScreenCols);
        Buf += COLOR_RESET;
        Buf += "\r\n";
    }

    const int EditRows = E.ScreenRows - 2; // title(1) + status(1)

    // ── Code / Overlay area (rows 2 .. ScreenRows-1) ─────────
    if (E.OverlayType != OverlayKind::None)
    {
        // Show overlay panel: title + scrollable lines
        const char* OverlayTitle =
            E.OverlayType == OverlayKind::TestOutput  ? " Test Results " :
            E.OverlayType == OverlayKind::PuzzleInfo  ? " Puzzle Info " :
                                                        " Help ";

        // First line: overlay title bar
        {
            std::string Bar = PadRight(OverlayTitle, E.ScreenCols);
            Buf += COLOR_HEADING;
            Buf += ANSI_REVERSE;
            Buf += TruncateTo(Bar, E.ScreenCols);
            Buf += COLOR_RESET;
            Buf += "\r\n";
        }

        const int ContentRows = EditRows - 1;
        for (int r = 0; r < ContentRows; ++r)
        {
            const int LineIdx = E.OverlayScroll + r;
            std::string Line;
            if (LineIdx < static_cast<int>(E.OverlayLines.size()))
            {
                Line = E.OverlayLines[static_cast<size_t>(LineIdx)];
            }
            const std::string Padded = PadRight(Line, E.ScreenCols);
            Buf += TruncateTo(Padded, E.ScreenCols);
            Buf += ANSI_CLEAR_EOL;
            Buf += "\r\n";
        }
    }
    else
    {
        // ── Normal code editor ────────────────────────────────
        const int LineNoWidth = 4; // "  1 "
        const int CodeWidth   = E.ScreenCols - LineNoWidth - 3; // " | " separator

        for (int r = 0; r < EditRows; ++r)
        {
            const int LineIdx = E.ScrollRow + r;
            const bool bCursorLine = (LineIdx == E.CursorRow);

            // Line-number column
            if (LineIdx < static_cast<int>(E.Lines.size()))
            {
                char LnBuf[16];
                std::snprintf(LnBuf, sizeof(LnBuf), "%3d", LineIdx + 1);
                Buf += COLOR_GRAY;
                Buf += LnBuf;
                Buf += " ";
                Buf += COLOR_RESET;

                if (bCursorLine)
                {
                    Buf += COLOR_INFO; // highlight current row separator
                    Buf += "| ";
                    Buf += COLOR_RESET;
                }
                else
                {
                    Buf += COLOR_GRAY;
                    Buf += "| ";
                    Buf += COLOR_RESET;
                }

                const std::string& CodeLine =
                    E.Lines[static_cast<size_t>(LineIdx)];

                if (bCursorLine && E.OverlayType == OverlayKind::None)
                {
                    // Render with cursor block
                    const int Col = E.CursorCol;
                    const std::string Before = CodeLine.substr(0, static_cast<size_t>(Col));
                    const std::string AtCursor =
                        (Col < static_cast<int>(CodeLine.size()))
                        ? std::string(1, CodeLine[static_cast<size_t>(Col)])
                        : " ";
                    const std::string After =
                        (Col < static_cast<int>(CodeLine.size()))
                        ? CodeLine.substr(static_cast<size_t>(Col) + 1)
                        : "";

                    Buf += TruncateTo(Before, CodeWidth);
                    // cursor block
                    if (static_cast<int>(Before.size()) < CodeWidth)
                    {
                        Buf += ANSI_REVERSE;
                        Buf += AtCursor[0];
                        Buf += COLOR_RESET;
                    }
                    // rest of line
                    const int Remaining = CodeWidth - static_cast<int>(Before.size()) - 1;
                    if (Remaining > 0)
                        Buf += TruncateTo(After, Remaining);
                }
                else
                {
                    Buf += TruncateTo(CodeLine, CodeWidth);
                }
            }
            else
            {
                // Empty row below code
                Buf += COLOR_GRAY;
                Buf += "    | ";
                Buf += COLOR_RESET;
            }

            Buf += ANSI_CLEAR_EOL;
            Buf += "\r\n";
        }
    }

    // ── Status / prompt bar (last row) ────────────────────────
    Buf += ANSI_REVERSE;
    if (E.bPromptActive)
    {
        std::string Prompt = " " + E.PromptLabel + E.PromptBuffer;
        Buf += PadRight(Prompt, E.ScreenCols);
    }
    else
    {
        // Check if status message is recent (< 4 s)
        const auto Now = std::chrono::steady_clock::now();
        const bool bShowStatus =
            !E.StatusMsg.empty() &&
            std::chrono::duration_cast<std::chrono::seconds>(Now - E.StatusMsgTime).count() < 4;

        if (bShowStatus)
        {
            Buf += PadRight(" " + E.StatusMsg, E.ScreenCols);
        }
        else if (E.OverlayType != OverlayKind::None)
        {
            const int Total = static_cast<int>(E.OverlayLines.size());
            const std::string ScrollInfo =
                " Line " + std::to_string(E.OverlayScroll + 1) +
                "/" + std::to_string(std::max(1, Total));
            const std::string Keys =
                "  Up/Down:Scroll  ^R:Run  ^P:Puzzle  F1:Help  Esc:Close";
            Buf += TruncateTo(PadRight(ScrollInfo + Keys, E.ScreenCols), E.ScreenCols);
        }
        else
        {
            const int TotalLines = static_cast<int>(E.Lines.size());
            const std::string Pos =
                " Ln " + std::to_string(E.CursorRow + 1) +
                "/" + std::to_string(TotalLines) +
                "  Col " + std::to_string(E.CursorCol + 1);
            const std::string Keys =
                "  ^S:Save  ^O:Open  ^R:Run  ^P:Puzzle  ^Q:Quit  F1:Help";
            Buf += TruncateTo(PadRight(Pos + Keys, E.ScreenCols), E.ScreenCols);
        }
    }
    Buf += COLOR_RESET;

    // Reposition system cursor where editor cursor is
    if (E.OverlayType == OverlayKind::None && !E.bPromptActive)
    {
        const int LineNoWidth = 4;
        const int SepWidth    = 2; // "| "
        const int VisualRow   = E.CursorRow - E.ScrollRow + 2; // 1=title, so +2
        const int VisualCol   = LineNoWidth + SepWidth + E.CursorCol + 1;
        Buf += AnsiMoveTo(VisualRow, VisualCol);
        Buf += ANSI_SHOW_CURSOR;
    }
    else
    {
        Buf += ANSI_SHOW_CURSOR;
    }

    std::cout << Buf;
    std::cout.flush();
}

// ============================================================
// Editor key handling
// ============================================================

// Returns false when the editor should close.
bool HandleEditorKey(EditorState& E, const KeyInput& Ki, const PuzzleData& Puzzle)
{
    // ── Prompt mode ──────────────────────────────────────────
    if (E.bPromptActive)
    {
        switch (Ki.Key)
        {
        case EditorKey::Enter:
        {
            const std::string Buf = E.PromptBuffer;
            E.bPromptActive = false;
            if (E.PromptDone) E.PromptDone(true, Buf);
            return true;
        }
        case EditorKey::Escape:
        case EditorKey::CtrlQ:
            E.bPromptActive = false;
            if (E.PromptDone) E.PromptDone(false, {});
            return true;
        case EditorKey::Backspace:
            if (!E.PromptBuffer.empty())
                E.PromptBuffer.pop_back();
            return true;
        case EditorKey::Regular:
            E.PromptBuffer += Ki.Ch;
            return true;
        default:
            return true;
        }
    }

    // ── Overlay mode ─────────────────────────────────────────
    if (E.OverlayType != OverlayKind::None)
    {
        const int MaxScroll =
            std::max(0, static_cast<int>(E.OverlayLines.size()) - (E.ScreenRows - 3));
        switch (Ki.Key)
        {
        case EditorKey::ArrowUp:
            E.OverlayScroll = std::max(0, E.OverlayScroll - 1);
            return true;
        case EditorKey::ArrowDown:
            E.OverlayScroll = std::min(MaxScroll, E.OverlayScroll + 1);
            return true;
        case EditorKey::PageUp:
            E.OverlayScroll = std::max(0, E.OverlayScroll - (E.ScreenRows - 4));
            return true;
        case EditorKey::PageDown:
            E.OverlayScroll = std::min(MaxScroll, E.OverlayScroll + (E.ScreenRows - 4));
            return true;
        case EditorKey::F1:
        case EditorKey::CtrlP:
            // Switching overlay type
            if (Ki.Key == EditorKey::F1)
            {
                E.OverlayLines = BuildHelpLines();
                E.OverlayType  = OverlayKind::Help;
            }
            else
            {
                E.OverlayLines = CollectPuzzleOverview(Puzzle);
                E.OverlayType  = OverlayKind::PuzzleInfo;
            }
            E.OverlayScroll = 0;
            return true;
        case EditorKey::CtrlR:
        case EditorKey::F5:
            E.OverlayLines  = CollectTestOutput(Puzzle, E.Lines, E.bDebugTrace);
            E.OverlayType   = OverlayKind::TestOutput;
            E.OverlayScroll = 0;
            return true;
        default:
            // Any other key closes the overlay
            E.OverlayType  = OverlayKind::None;
            E.OverlayLines = {};
            E.OverlayScroll= 0;
            return true;
        }
    }

    // ── Normal edit mode ──────────────────────────────────────
    switch (Ki.Key)
    {
    // -- Quit --------------------------------------------------
    case EditorKey::CtrlQ:
    case EditorKey::Escape:
        if (E.bModified)
        {
            E.bPromptActive = true;
            E.PromptLabel   = "Unsaved changes. Quit without saving? (y/n): ";
            E.PromptBuffer  = {};
            E.PromptDone    = [&E](bool bConfirmed, const std::string& Answer)
            {
                if (bConfirmed && !Answer.empty() &&
                    std::tolower(static_cast<unsigned char>(Answer[0])) == 'y')
                {
                    E.bQuitRequested = true;
                }
                else
                {
                    E.SetStatus("Quit cancelled.");
                }
            };
        }
        else
        {
            E.bQuitRequested = true;
        }
        return true;

    // -- Save --------------------------------------------------
    case EditorKey::CtrlS:
        if (E.Filename.empty())
        {
            E.bPromptActive = true;
            E.PromptLabel   = "Save to file: ";
            E.PromptBuffer  = {};
            E.PromptDone    = [&E](bool bConfirmed, const std::string& Name)
            {
                if (!bConfirmed || Name.empty())
                {
                    E.SetStatus("Save cancelled.");
                    return;
                }
                std::string Err;
                if (SaveCodeToFile(Name, E.Lines, Err))
                {
                    E.Filename = Name;
                    E.bModified = false;
                    E.SetStatus("Saved to " + Name);
                }
                else
                {
                    E.SetStatus("Error: " + Err);
                }
            };
        }
        else
        {
            std::string Err;
            if (SaveCodeToFile(E.Filename, E.Lines, Err))
            {
                E.bModified = false;
                E.SetStatus("Saved to " + E.Filename);
            }
            else
            {
                E.SetStatus("Error: " + Err);
            }
        }
        return true;

    // -- Open --------------------------------------------------
    case EditorKey::CtrlO:
        E.bPromptActive = true;
        E.PromptLabel   = "Open file: ";
        E.PromptBuffer  = E.Filename;
        E.PromptDone    = [&E](bool bConfirmed, const std::string& Name)
        {
            if (!bConfirmed || Name.empty())
            {
                E.SetStatus("Open cancelled.");
                return;
            }
            std::vector<std::string> Loaded;
            std::string Err;
            if (LoadCodeFromFile(Name, Loaded, Err))
            {
                E.Lines     = std::move(Loaded);
                E.Filename  = Name;
                E.bModified = false;
                E.CursorRow = 0;
                E.CursorCol = 0;
                E.ScrollRow = 0;
                E.ClampCursor();
                E.SetStatus("Loaded " + Name);
            }
            else
            {
                E.SetStatus("Error: " + Err);
            }
        };
        return true;

    // -- Run tests ---------------------------------------------
    case EditorKey::CtrlR:
    case EditorKey::F5:
        E.OverlayLines  = CollectTestOutput(Puzzle, E.Lines, E.bDebugTrace);
        E.OverlayType   = OverlayKind::TestOutput;
        E.OverlayScroll = 0;
        return true;

    // -- Puzzle overview ---------------------------------------
    case EditorKey::CtrlP:
        E.OverlayLines  = CollectPuzzleOverview(Puzzle);
        E.OverlayType   = OverlayKind::PuzzleInfo;
        E.OverlayScroll = 0;
        return true;

    // -- Help --------------------------------------------------
    case EditorKey::F1:
        E.OverlayLines  = BuildHelpLines();
        E.OverlayType   = OverlayKind::Help;
        E.OverlayScroll = 0;
        return true;

    // -- Toggle debug trace ------------------------------------
    case EditorKey::CtrlD:
        E.bDebugTrace = !E.bDebugTrace;
        E.SetStatus(std::string("Debug trace ") +
                    (E.bDebugTrace ? "enabled" : "disabled") + ".");
        return true;

    // -- Navigation --------------------------------------------
    case EditorKey::ArrowUp:
        if (E.CursorRow > 0)
        {
            --E.CursorRow;
            E.CursorCol = std::min(E.CursorCol,
                static_cast<int>(E.Lines[static_cast<size_t>(E.CursorRow)].size()));
        }
        return true;

    case EditorKey::ArrowDown:
        if (E.CursorRow < static_cast<int>(E.Lines.size()) - 1)
        {
            ++E.CursorRow;
            E.CursorCol = std::min(E.CursorCol,
                static_cast<int>(E.Lines[static_cast<size_t>(E.CursorRow)].size()));
        }
        return true;

    case EditorKey::ArrowLeft:
        if (E.CursorCol > 0)
        {
            --E.CursorCol;
        }
        else if (E.CursorRow > 0)
        {
            --E.CursorRow;
            E.CursorCol = static_cast<int>(
                E.Lines[static_cast<size_t>(E.CursorRow)].size());
        }
        return true;

    case EditorKey::ArrowRight:
    {
        const int LineLen = static_cast<int>(
            E.Lines[static_cast<size_t>(E.CursorRow)].size());
        if (E.CursorCol < LineLen)
        {
            ++E.CursorCol;
        }
        else if (E.CursorRow < static_cast<int>(E.Lines.size()) - 1)
        {
            ++E.CursorRow;
            E.CursorCol = 0;
        }
        return true;
    }

    case EditorKey::Home:
        E.CursorCol = 0;
        return true;

    case EditorKey::End:
        E.CursorCol = static_cast<int>(
            E.Lines[static_cast<size_t>(E.CursorRow)].size());
        return true;

    case EditorKey::PageUp:
    {
        const int PageSize = std::max(1, E.ScreenRows - 3);
        E.CursorRow = std::max(0, E.CursorRow - PageSize);
        E.ClampCursor();
        return true;
    }

    case EditorKey::PageDown:
    {
        const int PageSize = std::max(1, E.ScreenRows - 3);
        E.CursorRow = std::min(static_cast<int>(E.Lines.size()) - 1,
                               E.CursorRow + PageSize);
        E.ClampCursor();
        return true;
    }

    // -- Delete current line (Ctrl+K) -------------------------
    case EditorKey::CtrlK:
        if (!E.Lines.empty())
        {
            E.DeletedLine    = E.Lines[static_cast<size_t>(E.CursorRow)];
            E.DeletedLineRow = E.CursorRow;
            E.Lines.erase(E.Lines.begin() + E.CursorRow);
            if (E.Lines.empty()) E.Lines.push_back({});
            E.ClampCursor();
            E.bModified = true;
        }
        return true;

    // -- Restore last deleted line (Ctrl+U) -------------------
    case EditorKey::CtrlU:
        if (E.DeletedLineRow >= 0)
        {
            const size_t InsertAt = static_cast<size_t>(
                std::min(E.DeletedLineRow, static_cast<int>(E.Lines.size())));
            E.Lines.insert(E.Lines.begin() + static_cast<std::ptrdiff_t>(InsertAt),
                           E.DeletedLine);
            E.CursorRow = static_cast<int>(InsertAt);
            E.CursorCol = 0;
            E.DeletedLineRow = -1;
            E.bModified = true;
        }
        return true;

    // -- Editing -----------------------------------------------
    case EditorKey::Enter:
    {
        std::string& CurrentLine = E.Lines[static_cast<size_t>(E.CursorRow)];
        const std::string After = CurrentLine.substr(
            static_cast<size_t>(E.CursorCol));
        CurrentLine = CurrentLine.substr(0, static_cast<size_t>(E.CursorCol));
        E.Lines.insert(E.Lines.begin() + E.CursorRow + 1, After);
        ++E.CursorRow;
        E.CursorCol = 0;
        E.bModified = true;
        return true;
    }

    case EditorKey::Backspace:
    {
        if (E.CursorCol > 0)
        {
            std::string& L = E.Lines[static_cast<size_t>(E.CursorRow)];
            L.erase(static_cast<size_t>(E.CursorCol) - 1, 1);
            --E.CursorCol;
            E.bModified = true;
        }
        else if (E.CursorRow > 0)
        {
            // Merge with previous line
            const std::string Tail = E.Lines[static_cast<size_t>(E.CursorRow)];
            E.Lines.erase(E.Lines.begin() + E.CursorRow);
            --E.CursorRow;
            E.CursorCol = static_cast<int>(
                E.Lines[static_cast<size_t>(E.CursorRow)].size());
            E.Lines[static_cast<size_t>(E.CursorRow)] += Tail;
            E.bModified = true;
        }
        return true;
    }

    case EditorKey::DeleteKey:
    {
        std::string& L = E.Lines[static_cast<size_t>(E.CursorRow)];
        if (E.CursorCol < static_cast<int>(L.size()))
        {
            L.erase(static_cast<size_t>(E.CursorCol), 1);
            E.bModified = true;
        }
        else if (E.CursorRow < static_cast<int>(E.Lines.size()) - 1)
        {
            // Merge next line into current
            L += E.Lines[static_cast<size_t>(E.CursorRow) + 1];
            E.Lines.erase(E.Lines.begin() + E.CursorRow + 1);
            E.bModified = true;
        }
        return true;
    }

    case EditorKey::Tab:
    {
        std::string& L = E.Lines[static_cast<size_t>(E.CursorRow)];
        L.insert(static_cast<size_t>(E.CursorCol), "  ");
        E.CursorCol += 2;
        E.bModified = true;
        return true;
    }

    case EditorKey::Regular:
    {
        std::string& L = E.Lines[static_cast<size_t>(E.CursorRow)];
        L.insert(static_cast<size_t>(E.CursorCol), 1, Ki.Ch);
        ++E.CursorCol;
        E.bModified = true;
        return true;
    }

    default:
        return true;
    }
}

// ============================================================
// Main editor loop
// ============================================================

void RunEditor(EditorState& E, const PuzzleData& Puzzle)
{
    // Initial clear
    std::cout << ANSI_CLEAR_SCREEN;
    std::cout.flush();

    E.UpdateSize();
    E.ClampCursor();
    E.UpdateScroll();

    while (true)
    {
        E.UpdateSize();
        E.UpdateScroll();
        DrawEditorScreen(E);

        const KeyInput Ki = ReadKey();
        if (!HandleEditorKey(E, Ki, Puzzle)) break;

        if (E.bQuitRequested) break;
    }

    // Restore terminal
    std::cout << ANSI_SHOW_CURSOR;
    std::cout << ANSI_CLEAR_SCREEN;
    std::cout << AnsiMoveTo(1, 1);
    std::cout.flush();
}

} // anonymous namespace

// ============================================================
// main
// ============================================================

int main(int Argc, char* Argv[])
{
    const std::filesystem::path ExecutablePath =
        (Argc > 0) ? std::filesystem::path(Argv[0]) : std::filesystem::path();

    // ── Puzzle selection ──────────────────────────────────────
    std::string PuzzlePath;
    if (Argc >= 2)
    {
        PuzzlePath = Argv[1];
    }
    else
    {
        const std::filesystem::path PuzzlesDir = FindPuzzlesDirectory(ExecutablePath);
        if (PuzzlesDir.empty())
        {
            std::cout << COLOR_ERROR << "Could not find the Puzzles directory."
                      << COLOR_RESET << "\n"
                      << "Usage: conch_ide <puzzle.json> [optional_code_file]\n";
            return 1;
        }
        const std::filesystem::path Chosen = SelectPuzzleInteractively(PuzzlesDir);
        if (Chosen.empty())
        {
            std::cout << "No puzzle selected. Exiting.\n";
            return 0;
        }
        PuzzlePath = Chosen.string();
    }

    PuzzleData Puzzle;
    std::string Error;
    if (!LoadPuzzleFromFile(PuzzlePath, Puzzle, Error))
    {
        std::cout << COLOR_ERROR << "Failed to load puzzle: " << Error
                  << COLOR_RESET << "\n";
        return 1;
    }

    // ── Initial code ──────────────────────────────────────────
    EditorState E;
    E.PuzzleTitle = Puzzle.Title;
    E.Lines = Puzzle.StarterCode;

    if (Argc >= 3)
    {
        std::vector<std::string> Loaded;
        if (LoadCodeFromFile(Argv[2], Loaded, Error))
        {
            E.Lines    = std::move(Loaded);
            E.Filename = Argv[2];
        }
        else
        {
            std::cout << COLOR_ERROR << "Warning: " << Error
                      << "\nUsing starter code." << COLOR_RESET << "\n";
        }
    }

    if (E.Lines.empty()) E.Lines.push_back({});

    // ── Enter raw mode and run the editor ─────────────────────
    if (!EnableRawMode())
    {
        // Fallback: not a TTY (e.g. piped input) – just print code and exit.
        std::cout << COLOR_ERROR
                  << "Warning: could not enter interactive mode "
                     "(stdin is not a terminal)."
                  << COLOR_RESET << "\n";
        std::cout << COLOR_HEADING << "Loaded puzzle: " << Puzzle.Title
                  << COLOR_RESET << "\n";
        for (size_t i = 0; i < E.Lines.size(); ++i)
            std::cout << std::setw(3) << (i + 1) << " | " << E.Lines[i] << "\n";
        return 0;
    }

    RunEditor(E, Puzzle);
    DisableRawMode();

    // ── Post-exit summary ─────────────────────────────────────
    std::cout << COLOR_HEADING << "Session ended." << COLOR_RESET << "\n";
    if (E.bModified)
    {
        std::cout << COLOR_INFO << "(Code was modified but not saved.)"
                  << COLOR_RESET << "\n";
    }
    std::cout << COLOR_SUCCESS << "Good luck reducing those cycles!"
              << COLOR_RESET << "\n";
    return 0;
}

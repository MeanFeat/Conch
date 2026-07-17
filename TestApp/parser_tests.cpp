// parser_tests.cpp – standalone regression tests for the Conch parser.
//
// Build (from repo root):
//   g++ -std=c++17 TestApp/parser_tests.cpp src/Conchpiler/line.cpp \
//       src/Conchpiler/op.cpp src/Conchpiler/parser.cpp \
//       src/Conchpiler/scanner.cpp src/Conchpiler/thread.cpp \
//       src/Conchpiler/variable.cpp -I TestApp -I src -o /tmp/parser_tests
// Run:
//   /tmp/parser_tests

#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "../src/Conchpiler/parser.h"
#include "../src/Conchpiler/thread.h"
#include "../src/Conchpiler/variable.h"

namespace
{

struct TestResult
{
    std::string Name;
    bool Passed = false;
    std::string Reason;
};

// ── helpers ──────────────────────────────────────────────────────────────────

// Parse and execute a program, then return the OUT0 list contents (or empty on
// any error).  If ParseOk is not null it receives whether parsing succeeded.
std::vector<int32> RunProgram(
    const std::vector<std::string>& Lines,
    int32 InitX = 0,
    int32 InitY = 0,
    bool SetupOut0 = false,
    int32 Out0ExpectedSize = 16,
    bool* ParseOk = nullptr)
{
    ConParser Parser;
    ConThread Thread;
    if (!Parser.Parse(Lines, Thread))
    {
        if (ParseOk) *ParseOk = false;
        return {};
    }
    if (ParseOk) *ParseOk = true;

    Thread.SetTraceEnabled(false);
    Thread.SetThreadValue(0, InitX); // X
    Thread.SetThreadValue(1, InitY); // Y

    if (SetupOut0)
    {
        ConVariableList* Out0 = Thread.FindListVar("OUT0");
        if (Out0)
        {
            Out0->SetRole(ConListRole::Output);
            Out0->SetValues({});
            Out0->SetExpectedSize(static_cast<size_t>(Out0ExpectedSize));
            Out0->Reset();
        }
    }

    Thread.Execute();
    if (Thread.HadRuntimeError())
    {
        return {};
    }

    const ConVariableList* Out0 = Thread.FindListVar("OUT0");
    if (!Out0) return {};
    const std::vector<int32>& V = Out0->GetValues();
    return std::vector<int32>(V.begin(), V.end());
}

// Run the program and expect a specific error during *parsing*.
bool ExpectParseError(const std::vector<std::string>& Lines, const std::string& ExpectedFragment)
{
    ConParser Parser;
    ConThread Thread;
    if (Parser.Parse(Lines, Thread))
    {
        return false; // expected parse failure but it succeeded
    }
    for (const std::string& E : Parser.GetErrors())
    {
        if (E.find(ExpectedFragment) != std::string::npos)
            return true;
    }
    return false;
}

// ── individual tests ──────────────────────────────────────────────────────────

// Regression: SET OUT0 MUL X 2  (thread var * literal → OUT list)
TestResult Test_SetOut_MulThreadLiteral()
{
    TestResult R;
    R.Name = "SET OUT0 MUL X 2 (thread + literal)";

    bool ParseOk = false;
    // X=5 → MUL X 2 = 10 → OUT0 should contain [10]
    const std::vector<int32> Got = RunProgram(
        {"SET OUT0 MUL X 2", "RET"}, /*X=*/5, 0, true, 1, &ParseOk);

    if (!ParseOk)
    {
        R.Reason = "Parse failed (expected success)";
        return R;
    }
    if (Got != std::vector<int32>{10})
    {
        R.Reason = "Expected OUT0=[10], got wrong value";
        return R;
    }
    R.Passed = true;
    return R;
}

// Inline ADD with literal operands targeting OUT0: SET OUT0 ADD 3 4
TestResult Test_SetOut_AddLiteralLiteral()
{
    TestResult R;
    R.Name = "SET OUT0 ADD 3 4 (literal + literal)";

    bool ParseOk = false;
    const std::vector<int32> Got = RunProgram(
        {"SET OUT0 ADD 3 4", "RET"}, 0, 0, true, 1, &ParseOk);

    if (!ParseOk)
    {
        R.Reason = "Parse failed (expected success)";
        return R;
    }
    if (Got != std::vector<int32>{7})
    {
        R.Reason = "Expected OUT0=[7], got wrong value";
        return R;
    }
    R.Passed = true;
    return R;
}

// Mixed operands targeting OUT0: SET OUT0 ADD X Y
TestResult Test_SetOut_AddTwoThreadVars()
{
    TestResult R;
    R.Name = "SET OUT0 ADD X Y (thread + thread)";

    bool ParseOk = false;
    // X=3, Y=4 → ADD X Y = 7
    const std::vector<int32> Got = RunProgram(
        {"SET OUT0 ADD X Y", "RET"}, /*X=*/3, /*Y=*/4, true, 1, &ParseOk);

    if (!ParseOk)
    {
        R.Reason = "Parse failed (expected success)";
        return R;
    }
    if (Got != std::vector<int32>{7})
    {
        R.Reason = "Expected OUT0=[7], got wrong value";
        return R;
    }
    R.Passed = true;
    return R;
}

// Mixed literal+thread: SET OUT0 MUL 3 X
TestResult Test_SetOut_MulLiteralThread()
{
    TestResult R;
    R.Name = "SET OUT0 MUL 3 X (literal + thread)";

    bool ParseOk = false;
    // X=4 → MUL 3 4 = 12
    const std::vector<int32> Got = RunProgram(
        {"SET OUT0 MUL 3 X", "RET"}, /*X=*/4, 0, true, 1, &ParseOk);

    if (!ParseOk)
    {
        R.Reason = "Parse failed (expected success)";
        return R;
    }
    if (Got != std::vector<int32>{12})
    {
        R.Reason = "Expected OUT0=[12], got wrong value";
        return R;
    }
    R.Passed = true;
    return R;
}

// Thread destination still works: SET X MUL X 2
TestResult Test_SetThread_MulInline()
{
    TestResult R;
    R.Name = "SET X MUL X 2 (thread destination, unchanged)";

    ConParser Parser;
    ConThread Thread;
    const std::vector<std::string> Lines = {"SET X MUL X 2", "RET"};
    if (!Parser.Parse(Lines, Thread))
    {
        R.Reason = "Parse failed (expected success)";
        return R;
    }
    Thread.SetTraceEnabled(false);
    Thread.SetThreadValue(0, 5); // X=5
    Thread.Execute();
    if (Thread.HadRuntimeError())
    {
        R.Reason = "Runtime error (unexpected)";
        return R;
    }
    if (Thread.GetThreadValue(0) != 10)
    {
        R.Reason = "Expected X=10, got " + std::to_string(Thread.GetThreadValue(0));
        return R;
    }
    R.Passed = true;
    return R;
}

// DAT list as inline SET destination must still be rejected.
TestResult Test_SetDat_Rejected()
{
    TestResult R;
    R.Name = "SET DAT0 MUL X 2 (DAT destination rejected)";

    if (!ExpectParseError({"SET DAT0 MUL X 2", "RET"}, "SET destination"))
    {
        R.Reason = "Expected a parse error about SET destination, but got none (or wrong message)";
        return R;
    }
    R.Passed = true;
    return R;
}

// Multiple OUT appends in a loop work correctly.
TestResult Test_SetOut_MultipleAppends()
{
    TestResult R;
    R.Name = "Multiple SET OUT0 MUL X 2 in loop";

    // Program: multiply each element of X by 2 using inline form
    // (manually unrolled for simplicity – no DAT input needed)
    bool ParseOk = false;
    const std::vector<std::string> Lines = {
        "SET OUT0 MUL 1 2",
        "SET OUT0 MUL 2 2",
        "SET OUT0 MUL 3 2",
        "RET"
    };
    const std::vector<int32> Got = RunProgram(Lines, 0, 0, true, 3, &ParseOk);

    if (!ParseOk)
    {
        R.Reason = "Parse failed (expected success)";
        return R;
    }
    if (Got != std::vector<int32>{2, 4, 6})
    {
        R.Reason = "Expected OUT0=[2,4,6], got wrong values";
        return R;
    }
    R.Passed = true;
    return R;
}

TestResult Test_IfSingleVariableTruthy()
{
    TestResult R;
    R.Name = "IF X uses non-zero truthiness";

    bool ParseOk = false;
    const std::vector<std::string> Lines = {
        "IF X",
        "  SET OUT0 1",
        "RET"
    };

    const std::vector<int32> FalseCase = RunProgram(Lines, 0, 0, true, 1, &ParseOk);
    if (!ParseOk)
    {
        R.Reason = "Parse failed for IF X";
        return R;
    }
    if (!FalseCase.empty())
    {
        R.Reason = "Expected IF X to skip block when X=0";
        return R;
    }

    const std::vector<int32> TrueCase = RunProgram(Lines, 5, 0, true, 1, &ParseOk);
    if (!ParseOk)
    {
        R.Reason = "Parse failed for IF X with non-zero input";
        return R;
    }
    if (TrueCase != std::vector<int32>{1})
    {
        R.Reason = "Expected IF X to execute block when X is non-zero";
        return R;
    }

    R.Passed = true;
    return R;
}

TestResult Test_IfnSingleVariableTruthy()
{
    TestResult R;
    R.Name = "IFN X inverts non-zero truthiness";

    bool ParseOk = false;
    const std::vector<std::string> Lines = {
        "IFN X",
        "  SET OUT0 1",
        "RET"
    };

    const std::vector<int32> TrueCase = RunProgram(Lines, 0, 0, true, 1, &ParseOk);
    if (!ParseOk)
    {
        R.Reason = "Parse failed for IFN X";
        return R;
    }
    if (TrueCase != std::vector<int32>{1})
    {
        R.Reason = "Expected IFN X to execute block when X=0";
        return R;
    }

    const std::vector<int32> FalseCase = RunProgram(Lines, 5, 0, true, 1, &ParseOk);
    if (!ParseOk)
    {
        R.Reason = "Parse failed for IFN X with non-zero input";
        return R;
    }
    if (!FalseCase.empty())
    {
        R.Reason = "Expected IFN X to skip block when X is non-zero";
        return R;
    }

    R.Passed = true;
    return R;
}

} // namespace

int main()
{
    std::vector<TestResult> Results;
    Results.push_back(Test_SetOut_MulThreadLiteral());
    Results.push_back(Test_SetOut_AddLiteralLiteral());
    Results.push_back(Test_SetOut_AddTwoThreadVars());
    Results.push_back(Test_SetOut_MulLiteralThread());
    Results.push_back(Test_SetThread_MulInline());
    Results.push_back(Test_SetDat_Rejected());
    Results.push_back(Test_SetOut_MultipleAppends());
    Results.push_back(Test_IfSingleVariableTruthy());
    Results.push_back(Test_IfnSingleVariableTruthy());

    int Passed = 0;
    int Failed = 0;
    for (const TestResult& R : Results)
    {
        const char* Status = R.Passed ? "PASS" : "FAIL";
        std::cout << "[" << Status << "] " << R.Name;
        if (!R.Passed)
            std::cout << "\n       Reason: " << R.Reason;
        std::cout << "\n";
        if (R.Passed) ++Passed; else ++Failed;
    }

    std::cout << "\n" << Passed << "/" << (Passed + Failed) << " tests passed.\n";
    return Failed == 0 ? 0 : 1;
}

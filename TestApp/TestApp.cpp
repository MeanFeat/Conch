#include "../src/Conchpiler/parser.h"
#include <iostream>
#include <string>
#include <vector>

int main(int Argc, char* Argv[])
{
    ConParser Parser;
    std::vector<std::string> Lines = {
        "SET X 10",
        "SET Y 10",
        "ADD X Y",
        "SUB Y 15",
        "SET Z MUL Y -2",
        "IF GTR X Y",
        "   ADD X Y",
        "LOOP GTR X 0",
        "   DECR X",
        "   REDO",
        "SWP X",
        "SET X SUB Z 2"
    };
    ConThread Thread;
    const bool bParsed = Parser.Parse(Lines, Thread);
    if (!bParsed)
    {
        std::cout << "Failed to parse program:\n";
        for (const std::string& Error : Parser.GetErrors())
        {
            std::cout << "  " << Error << std::endl;
        }
        return 1;
    }

    Thread.UpdateCycleCount();
    Thread.Execute();
    if (Thread.HadRuntimeError())
    {
        std::cout << "Runtime error(s) encountered:\n";
        for (const std::string& Error : Thread.GetRuntimeErrors())
        {
            std::cout << "  " << Error << std::endl;
        }
        return 1;
    }
    std::cout << "Total cycles: " << Thread.GetCycleCount() << std::endl;
    return 0;
}

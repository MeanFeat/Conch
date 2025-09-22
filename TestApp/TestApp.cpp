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
    ConThread Thread = Parser.Parse(Lines);
    Thread.UpdateCycleCount();
    Thread.Execute();
    std::cout << "Total cycles: " << Thread.GetCycleCount() << std::endl;
    return 0;
}

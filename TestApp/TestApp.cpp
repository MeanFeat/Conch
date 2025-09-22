#include "../src/Conchpiler/parser.h"
#include <iostream>
#include <optional>
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
    std::optional<ConThread> ThreadResult = Parser.Parse(Lines);
    if (!ThreadResult)
    {
        std::cout << "Failed to parse program:\n";
        for (const std::string& Error : Parser.GetErrors())
        {
            std::cout << "  " << Error << std::endl;
        }
        return 1;
    }

    ThreadResult->UpdateCycleCount();
    ThreadResult->Execute();
    std::cout << "Total cycles: " << ThreadResult->GetCycleCount() << std::endl;
    return 0;
}

#include "../src/Conchpiler/thread.h"
#include <vector>
struct ConBaseOp;

int main(int Argc, char* Argv[])
{
    ConVariableCached X;
    ConVariableCached Y;
    ConVariableCached Z;
    ConVariableAbsolute Five(5);
    ConVariableAbsolute Ten(10);
    ConThread Thread({&X,&Y});
    ConSetOp Set_X_10({&X, &Ten});
    ConSetOp Set_Y_10({&Y, &Ten});
    ConAddOp AddYToX({&X, &Y});
    ConSubOp Sub5FromY({&Y, &Five});
    ConSwpOp SwpX({&X});
    Thread.ConstructLine({&Set_X_10});
    Thread.ConstructLine({&Set_Y_10});
    Thread.ConstructLine({&AddYToX});
    Thread.ConstructLine({&Sub5FromY});
    Thread.ConstructLine({&AddYToX});
    Thread.ConstructLine({&SwpX});
    Thread.Execute();
    
    return 0;
}

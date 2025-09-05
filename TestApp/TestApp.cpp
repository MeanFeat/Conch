#include "../src/Conchpiler/thread.h"

int main(int argc, char* argv[])
{
    ConThread Thread;

    ConVariableCached X;
    ConVariableCached Y;
    ConVariableCached Z;

    Thread.SetVariables({&X,&Y,&Z});
    
    vector<ConBaseOp*> SetX;
    vector<ConBaseOp*> SetY;
    ConSetOp Set_X_10;
    ConSetOp Set_Y_10;
    ConVariableAbsolute Ten(10);
    Set_X_10.SetArgs({&X, &Ten});
    Set_Y_10.SetArgs({&Y, &Ten});
    SetX.push_back(&Set_X_10);
    SetY.push_back(&Set_Y_10);
    Thread.ConstructLine(SetX);
    Thread.ConstructLine(SetY);
    ConAddOp AddYToX;
    AddYToX.SetArgs({&X, &Y});
    Thread.ConstructLine({&AddYToX});

    Thread.Execute();
    
    return 0;
}

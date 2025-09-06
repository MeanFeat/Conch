#include "../src/Conchpiler/thread.h"

int main(int argc, char* argv[])
{
    ConThread Thread;
    ConVariableCached X;
    ConVariableCached Y;
    ConVariableCached Z;
    vector<ConBaseOp*> SetX;
    vector<ConBaseOp*> SetY;
    ConSetOp Set_X_10;
    ConSetOp Set_Y_10;
    ConVariableAbsolute Ten(10);
    Thread.SetVariables({&X,&Y,&Z});
    Set_X_10.SetArgs({&X, &Ten});
    Set_Y_10.SetArgs({&Y, &Ten});
    SetX.push_back(&Set_X_10);
    SetY.push_back(&Set_Y_10);
    ConAddOp AddYToX;
    AddYToX.SetArgs({&X, &Y});
    ConSubOp Sub5fromY;
    ConVariableAbsolute Five(5);
    Sub5fromY.SetArgs({&Y, &Five});
    Thread.ConstructLine(SetX);
    Thread.ConstructLine(SetY);
    Thread.ConstructLine({&AddYToX});
    Thread.ConstructLine({&Sub5fromY});
    Thread.ConstructLine({&AddYToX});
    Thread.Execute();
    
    return 0;
}

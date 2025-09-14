#include "../src/Conchpiler/thread.h"
#include <vector>
struct ConBaseOp;

int main(int Argc, char* Argv[])
{
    ConVariableCached X;
    ConVariableCached Y;
    vector<ConVariable*> regs = {&X, &Y};
    ConThread Thread(regs);
    ConSetOp Set_X_10({{OperandKind::Reg, 0}, {OperandKind::Imm, 10}});
    ConSetOp Set_Y_10({{OperandKind::Reg, 1}, {OperandKind::Imm, 10}});
    ConAddOp AddYToX({{OperandKind::Reg, 0}, {OperandKind::Reg, 0}, {OperandKind::Reg, 1}});
    ConSubOp Sub5FromY({{OperandKind::Reg, 1}, {OperandKind::Reg, 1}, {OperandKind::Imm, 5}});
    ConSwpOp SwpX({{OperandKind::Reg, 0}});
    Thread.ConstructLine({&Set_X_10});
    Thread.ConstructLine({&Set_Y_10});
    Thread.ConstructLine({&AddYToX});
    Thread.ConstructLine({&Sub5FromY});
    Thread.ConstructLine({&AddYToX});
    Thread.ConstructLine({&SwpX});
    Thread.Execute(regs);
    
    return 0;
}

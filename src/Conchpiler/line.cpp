#include "line.h"
class ConBasOp;


inline void ConLine::Execute()
{
    for (ConBasOp& Op : Ops)
    {
        Op.Execute();
    }
}

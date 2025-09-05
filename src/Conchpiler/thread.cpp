#include "common.h"
#include "thread.h"

inline ConThread::~ConThread()
{
}

inline void ConThread::Execute()
{
    for (ConLine& Line : Lines)
    {
        Line.Execute();
    }
}
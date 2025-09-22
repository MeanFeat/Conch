#pragma once
#include "common.h"

struct ConCompilable 
{
    virtual ~ConCompilable() = default;
    virtual void Execute() = 0;
    virtual void UpdateCycleCount() { ResetCycleCount(); };
    
    void AddCycles(int32 Cycles);
    int32 GetCycleCount() const { return CycleCount; }
    int32 ResetCycleCount() { return CycleCount = 0; }
    
private:
    int32 CycleCount = 0;
};

inline void ConCompilable::AddCycles(const int32 Cycles)
{
    CycleCount += Cycles;   
}

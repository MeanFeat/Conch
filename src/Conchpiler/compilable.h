#pragma once
#include "common.h"

struct abstract ConCompilable 
{
    virtual ~ConCompilable() = default;
    virtual void Execute() = 0;
    virtual void UpdateCycleCount() = 0;
    virtual int32 GetCycleCount() const { return CycleCount; }
    private:
    int32 CycleCount = 0;
};
#pragma once
#include "common.h"

class ConVariable
{
    ConVariable() = default;

    int32 GetVal() const;
    void SetVal(int32 NewVal);
    int32 GetCache() const;
    void Swap();
    ConVariable SwapInPlace();
    
private:
    
    int32 Val = 0;
    int32 Cache = 0;
};

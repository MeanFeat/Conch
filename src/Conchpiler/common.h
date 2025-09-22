#pragma once

#include <vector>

using namespace std;

typedef int int32;

struct ConSourceLocation
{
    int32 Line = 0;
    int32 Column = 0;

    bool IsValid() const
    {
        return Line > 0;
    }
};


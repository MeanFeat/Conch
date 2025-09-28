#pragma once

#include "SimpleJson.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct PuzzleListSpec
{
    int Index = 0;
    std::vector<int> Values;
};

struct PuzzleExpectation
{
    std::unordered_map<std::string, int> Registers;
    std::vector<PuzzleListSpec> Lists;

    bool HasExpectations() const
    {
        return !Registers.empty() || !Lists.empty();
    }
};

struct PuzzleTestCase
{
    std::string Name;
    std::unordered_map<std::string, int> InitialRegisters;
    std::vector<PuzzleListSpec> InitialLists;
    PuzzleExpectation Expectation;
};

struct PuzzleHistoryEntry
{
    std::string Label;
    int Cycles = 0;
};

struct PuzzleData
{
    std::string Title;
    std::string Description;
    std::vector<std::string> StarterCode;
    std::vector<PuzzleTestCase> Tests;
    std::vector<PuzzleHistoryEntry> History;
};

bool LoadPuzzleFromFile(const std::string& Path, PuzzleData& OutData, std::string& OutError);

std::optional<int> ParseRegisterName(const std::string& Name);


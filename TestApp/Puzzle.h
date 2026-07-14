#pragma once

#include "SimpleJson.h"

#include <string>
#include <unordered_map>
#include <vector>

struct PuzzleListSpec
{
    std::string Name;
    std::vector<int> Values;
};

struct PuzzleOutSpec
{
    std::string Name;
    int ExpectedSize = 0;
};

struct PuzzleExpectation
{
    std::unordered_map<std::string, int> Registers;
    std::vector<PuzzleListSpec> ExpectedOut;

    bool HasExpectations() const
    {
        return !Registers.empty() || !ExpectedOut.empty();
    }
};

struct PuzzleTestCase
{
    std::string Name;
    std::unordered_map<std::string, int> InitialRegisters;
    std::vector<PuzzleListSpec> DatInputs;
    std::vector<PuzzleOutSpec> OutSpecs;
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

bool ParseRegisterName(const std::string& Name, int& OutIndex);


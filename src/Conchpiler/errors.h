#pragma once

#include "common.h"
#include "scanner.h"

#include <sstream>
#include <stdexcept>
#include <string>

inline std::string FormatErrorMessage(const ConSourceLocation& Location, const std::string& Message)
{
    std::ostringstream Stream;
    if (Location.IsValid())
    {
        Stream << "[line " << Location.Line << ", col " << Location.Column << "] ";
    }
    Stream << Message;
    return Stream.str();
}

struct ConParseError : public std::runtime_error
{
    ConParseError(const Token& InToken, const std::string& InMessage)
        : std::runtime_error(InMessage)
        , TokenInfo(InToken)
    {
        Location.Line = InToken.Line;
        Location.Column = InToken.Column;
    }

    Token TokenInfo;
    ConSourceLocation Location;
};

struct ConRuntimeError : public std::runtime_error
{
    ConRuntimeError(const ConSourceLocation& InLocation, const std::string& InMessage)
        : std::runtime_error(InMessage)
        , Location(InLocation)
    {
    }

    ConSourceLocation Location;
};


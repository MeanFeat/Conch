#include "SimpleJson.h"

#include <cctype>
#include <sstream>

namespace
{
class SimpleJsonParser
{
public:
    explicit SimpleJsonParser(const std::string& InText)
        : Text(InText)
    {
    }

    bool Parse(SimpleJsonValue& OutValue)
    {
        SkipWhitespace();
        if (!ParseValue(OutValue))
        {
            return false;
        }
        SkipWhitespace();
        if (!IsAtEnd())
        {
            ErrorMessage = "Unexpected trailing characters";
            return false;
        }
        return true;
    }

    const std::string& GetError() const
    {
        return ErrorMessage;
    }

private:
    bool IsAtEnd() const
    {
        return Index >= Text.size();
    }

    char Peek() const
    {
        return IsAtEnd() ? '\0' : Text[Index];
    }

    char Consume()
    {
        return IsAtEnd() ? '\0' : Text[Index++];
    }

    void SkipWhitespace()
    {
        while (!IsAtEnd())
        {
            const char Ch = Peek();
            if (Ch == ' ' || Ch == '\t' || Ch == '\n' || Ch == '\r')
            {
                ++Index;
            }
            else
            {
                break;
            }
        }
    }

    bool Match(const char Expected)
    {
        if (Peek() == Expected)
        {
            ++Index;
            return true;
        }
        return false;
    }

    bool ParseValue(SimpleJsonValue& OutValue)
    {
        if (IsAtEnd())
        {
            ErrorMessage = "Unexpected end of input";
            return false;
        }
        const char Ch = Peek();
        if (Ch == 'n')
        {
            return ParseNull(OutValue);
        }
        if (Ch == 't' || Ch == 'f')
        {
            return ParseBool(OutValue);
        }
        if (Ch == '"')
        {
            return ParseString(OutValue);
        }
        if (Ch == '[')
        {
            return ParseArray(OutValue);
        }
        if (Ch == '{')
        {
            return ParseObject(OutValue);
        }
        return ParseNumber(OutValue);
    }

    bool ParseNull(SimpleJsonValue& OutValue)
    {
        if (Text.compare(Index, 4, "null") != 0)
        {
            ErrorMessage = "Invalid token, expected 'null'";
            return false;
        }
        Index += 4;
        OutValue.SetNull();
        return true;
    }

    bool ParseBool(SimpleJsonValue& OutValue)
    {
        if (Text.compare(Index, 4, "true") == 0)
        {
            Index += 4;
            OutValue.SetBool(true);
            return true;
        }
        if (Text.compare(Index, 5, "false") == 0)
        {
            Index += 5;
            OutValue.SetBool(false);
            return true;
        }
        ErrorMessage = "Invalid boolean literal";
        return false;
    }

    bool ParseString(SimpleJsonValue& OutValue)
    {
        if (!Match('"'))
        {
            ErrorMessage = "Expected opening quote for string";
            return false;
        }
        std::string Value;
        while (!IsAtEnd())
        {
            const char Ch = Consume();
            if (Ch == '"')
            {
                OutValue.SetString(Value);
                return true;
            }
            if (Ch == '\\')
            {
                if (!ParseEscape(Value))
                {
                    return false;
                }
            }
            else
            {
                Value.push_back(Ch);
            }
        }
        ErrorMessage = "Unterminated string";
        return false;
    }

    bool ParseEscape(std::string& OutValue)
    {
        if (IsAtEnd())
        {
            ErrorMessage = "Unterminated escape sequence";
            return false;
        }
        const char Escaped = Consume();
        switch (Escaped)
        {
        case '"': OutValue.push_back('"'); return true;
        case '\\': OutValue.push_back('\\'); return true;
        case '/': OutValue.push_back('/'); return true;
        case 'b': OutValue.push_back('\b'); return true;
        case 'f': OutValue.push_back('\f'); return true;
        case 'n': OutValue.push_back('\n'); return true;
        case 'r': OutValue.push_back('\r'); return true;
        case 't': OutValue.push_back('\t'); return true;
        case 'u':
            return ParseUnicodeEscape(OutValue);
        default:
            ErrorMessage = "Unsupported escape sequence";
            return false;
        }
    }

    bool ParseUnicodeEscape(std::string& OutValue)
    {
        if (Index + 4 > Text.size())
        {
            ErrorMessage = "Invalid unicode escape";
            return false;
        }
        int CodePoint = 0;
        for (int i = 0; i < 4; ++i)
        {
            const char Ch = Text[Index + i];
            if (!std::isxdigit(static_cast<unsigned char>(Ch)))
            {
                ErrorMessage = "Invalid unicode escape";
                return false;
            }
            CodePoint <<= 4;
            if (Ch >= '0' && Ch <= '9')
            {
                CodePoint += Ch - '0';
            }
            else if (Ch >= 'a' && Ch <= 'f')
            {
                CodePoint += 10 + (Ch - 'a');
            }
            else
            {
                CodePoint += 10 + (Ch - 'A');
            }
        }
        Index += 4;
        if (CodePoint <= 0x7F)
        {
            OutValue.push_back(static_cast<char>(CodePoint));
        }
        else
        {
            std::ostringstream Oss;
            Oss << "\\u" << std::hex << CodePoint;
            OutValue += Oss.str();
        }
        return true;
    }

    bool ParseNumber(SimpleJsonValue& OutValue)
    {
        const size_t Start = Index;
        if (Peek() == '-')
        {
            ++Index;
        }
        if (!std::isdigit(static_cast<unsigned char>(Peek())))
        {
            ErrorMessage = "Invalid number literal";
            return false;
        }
        if (Peek() == '0')
        {
            ++Index;
        }
        else
        {
            while (std::isdigit(static_cast<unsigned char>(Peek())))
            {
                ++Index;
            }
        }

        if (Peek() == '.')
        {
            ++Index;
            if (!std::isdigit(static_cast<unsigned char>(Peek())))
            {
                ErrorMessage = "Invalid fractional literal";
                return false;
            }
            while (std::isdigit(static_cast<unsigned char>(Peek())))
            {
                ++Index;
            }
        }

        if (Peek() == 'e' || Peek() == 'E')
        {
            ++Index;
            if (Peek() == '+' || Peek() == '-')
            {
                ++Index;
            }
            if (!std::isdigit(static_cast<unsigned char>(Peek())))
            {
                ErrorMessage = "Invalid exponent";
                return false;
            }
            while (std::isdigit(static_cast<unsigned char>(Peek())))
            {
                ++Index;
            }
        }

        const std::string NumberLiteral = Text.substr(Start, Index - Start);
        try
        {
            const double Value = std::stod(NumberLiteral);
            OutValue.SetNumber(Value);
            return true;
        }
        catch (...)
        {
            ErrorMessage = "Failed to parse number";
            return false;
        }
    }

    bool ParseArray(SimpleJsonValue& OutValue)
    {
        if (!Match('['))
        {
            ErrorMessage = "Expected '['";
            return false;
        }
        SimpleJsonValue::Array Elements;
        SkipWhitespace();
        if (Match(']'))
        {
            OutValue.SetArray(std::move(Elements));
            return true;
        }
        while (true)
        {
            SimpleJsonValue Element;
            if (!ParseValue(Element))
            {
                return false;
            }
            Elements.push_back(Element);
            SkipWhitespace();
            if (Match(']'))
            {
                OutValue.SetArray(std::move(Elements));
                return true;
            }
            if (!Match(','))
            {
                ErrorMessage = "Expected ',' or ']'";
                return false;
            }
            SkipWhitespace();
        }
    }

    bool ParseObject(SimpleJsonValue& OutValue)
    {
        if (!Match('{'))
        {
            ErrorMessage = "Expected '{'";
            return false;
        }
        SimpleJsonValue::Object Members;
        SkipWhitespace();
        if (Match('}'))
        {
            OutValue.SetObject(std::move(Members));
            return true;
        }
        while (true)
        {
            SimpleJsonValue KeyValue;
            if (!ParseString(KeyValue))
            {
                return false;
            }
            SkipWhitespace();
            if (!Match(':'))
            {
                ErrorMessage = "Expected ':' after key";
                return false;
            }
            SkipWhitespace();
            SimpleJsonValue Value;
            if (!ParseValue(Value))
            {
                return false;
            }
            Members[KeyValue.AsString()] = Value;
            SkipWhitespace();
            if (Match('}'))
            {
                OutValue.SetObject(std::move(Members));
                return true;
            }
            if (!Match(','))
            {
                ErrorMessage = "Expected ',' or '}'";
                return false;
            }
            SkipWhitespace();
        }
    }

    const std::string& Text;
    size_t Index = 0;
    std::string ErrorMessage;
};
}

SimpleJsonValue::SimpleJsonValue()
    : ValueType(Type::Null)
    , BoolValue(false)
    , NumberValue(0.0)
{
}

bool SimpleJsonValue::AsBool(const bool Default) const
{
    return IsBool() ? BoolValue : Default;
}

double SimpleJsonValue::AsNumber(const double Default) const
{
    return IsNumber() ? NumberValue : Default;
}

int SimpleJsonValue::AsInt(const int Default) const
{
    return static_cast<int>(AsNumber(static_cast<double>(Default)));
}

std::string SimpleJsonValue::AsString(const std::string& Default) const
{
    return IsString() ? StringValue : Default;
}

SimpleJsonValue::Array& SimpleJsonValue::GetArray()
{
    return ArrayValue;
}

const SimpleJsonValue::Array& SimpleJsonValue::GetArray() const
{
    static Array Empty;
    return IsArray() ? ArrayValue : Empty;
}

SimpleJsonValue::Object& SimpleJsonValue::GetObject()
{
    return ObjectValue;
}

const SimpleJsonValue::Object& SimpleJsonValue::GetObject() const
{
    static Object Empty;
    return IsObject() ? ObjectValue : Empty;
}

const SimpleJsonValue* SimpleJsonValue::Find(const std::string& Key) const
{
    if (!IsObject())
    {
        return nullptr;
    }
    const auto It = ObjectValue.find(Key);
    return It != ObjectValue.end() ? &It->second : nullptr;
}

SimpleJsonValue* SimpleJsonValue::Find(const std::string& Key)
{
    if (!IsObject())
    {
        return nullptr;
    }
    auto It = ObjectValue.find(Key);
    return It != ObjectValue.end() ? &It->second : nullptr;
}

bool SimpleJsonValue::Parse(const std::string& Text, SimpleJsonValue& OutValue, std::string& OutError)
{
    SimpleJsonParser Parser(Text);
    if (!Parser.Parse(OutValue))
    {
        OutError = Parser.GetError();
        return false;
    }
    return true;
}

void SimpleJsonValue::SetNull()
{
    ValueType = Type::Null;
    BoolValue = false;
    NumberValue = 0.0;
    StringValue.clear();
    ArrayValue.clear();
    ObjectValue.clear();
}

void SimpleJsonValue::SetBool(const bool bValue)
{
    ValueType = Type::Boolean;
    BoolValue = bValue;
}

void SimpleJsonValue::SetNumber(const double Number)
{
    ValueType = Type::Number;
    NumberValue = Number;
}

void SimpleJsonValue::SetString(std::string Value)
{
    ValueType = Type::String;
    StringValue = std::move(Value);
}

void SimpleJsonValue::SetArray(Array&& Values)
{
    ValueType = Type::Array;
    ArrayValue = std::move(Values);
}

void SimpleJsonValue::SetObject(Object&& Members)
{
    ValueType = Type::Object;
    ObjectValue = std::move(Members);
}


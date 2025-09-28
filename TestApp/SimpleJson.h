#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

class SimpleJsonValue
{
public:
    enum class Type
    {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    using Array = std::vector<SimpleJsonValue>;
    using Object = std::map<std::string, SimpleJsonValue>;

    SimpleJsonValue();

    Type GetType() const { return ValueType; }
    bool IsNull() const { return ValueType == Type::Null; }
    bool IsBool() const { return ValueType == Type::Boolean; }
    bool IsNumber() const { return ValueType == Type::Number; }
    bool IsString() const { return ValueType == Type::String; }
    bool IsArray() const { return ValueType == Type::Array; }
    bool IsObject() const { return ValueType == Type::Object; }

    bool AsBool(bool Default = false) const;
    double AsNumber(double Default = 0.0) const;
    int AsInt(int Default = 0) const;
    std::string AsString(const std::string& Default = "") const;

    Array& GetArray();
    const Array& GetArray() const;

    Object& GetObject();
    const Object& GetObject() const;

    const SimpleJsonValue* Find(const std::string& Key) const;
    SimpleJsonValue* Find(const std::string& Key);

    static bool Parse(const std::string& Text, SimpleJsonValue& OutValue, std::string& OutError);

    void SetNull();
    void SetBool(bool bValue);
    void SetNumber(double Number);
    void SetString(std::string Value);
    void SetArray(Array&& Values);
    void SetObject(Object&& Members);

private:
    Type ValueType;
    bool BoolValue;
    double NumberValue;
    std::string StringValue;
    Array ArrayValue;
    Object ObjectValue;
};


#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <cstdint>

struct CityCommand
{
    enum class Type
    {
        RESURRECT,
        HEAL,
        BUY,
        SELL,
        DEPOSIT,
        WITHDRAW,
        LIST,
    };

    Type type;
    std::string itemName;
    uint32_t goldAmount = 0;
    bool isGold = false;
};

class CityCommandParser
{
public:
    std::optional<CityCommand> parse(const std::string &raw) const;
};
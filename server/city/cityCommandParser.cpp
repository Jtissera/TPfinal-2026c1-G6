#include "cityCommandParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

static std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return s;
}

std::optional<CityCommand> CityCommandParser::parse(const std::string &raw)
{
    if (raw.empty() || raw[0] != '/')
        return std::nullopt;

    std::istringstream ss(raw.substr(1));
    std::string verb;
    ss >> verb;
    verb = toLower(verb);

    CityCommand cmd;

    if (verb == "resucitar")
    {
        cmd.type = CityCommand::Type::RESURRECT;
        return cmd;
    }

    if (verb == "curar")
    {
        cmd.type = CityCommand::Type::HEAL;
        return cmd;
    }

    if (verb == "comprar")
    {
        std::string item;
        if (!(ss >> item))
            return std::nullopt;
        cmd.type = CityCommand::Type::BUY;
        cmd.itemName = toLower(item);
        return cmd;
    }

    if (verb == "vender")
    {
        std::string item;
        if (!(ss >> item))
            return std::nullopt;
        cmd.type = CityCommand::Type::SELL;
        cmd.itemName = toLower(item);
        return cmd;
    }

    auto parseGoldOrItem = [&](CityCommand::Type type) -> std::optional<CityCommand>
    {
        std::string first;
        if (!(ss >> first))
            return std::nullopt;
        cmd.type = type;
        if (toLower(first) == "oro")
        {
            uint32_t amount = 0;
            if (!(ss >> amount) || amount == 0)
                return std::nullopt;
            cmd.isGold = true;
            cmd.goldAmount = amount;
        }
        else
        {
            cmd.itemName = toLower(first);
        }
        return cmd;
    };

    if (verb == "depositar")
        return parseGoldOrItem(CityCommand::Type::DEPOSIT);
    if (verb == "retirar")
        return parseGoldOrItem(CityCommand::Type::WITHDRAW);

    if (verb == "lista")
    {
        cmd.type = CityCommand::Type::LIST;
        return cmd;
    }

    return std::nullopt;
}
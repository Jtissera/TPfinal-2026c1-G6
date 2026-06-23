#include "cityCommandParser.h"
#include <algorithm>
#include <cctype>

namespace
{
    std::string toLower(const std::string &s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });
        return out;
    }

    std::optional<CityCommand> parseGoldOrItem(CityCommand::Type type,
                                               const std::string &first,
                                               std::istringstream &ss)
    {
        CityCommand cmd;
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
    }
}

std::optional<CityCommand> CityCommandParser::parse(const std::string &raw) const
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
    if (verb == "lista")
    {
        cmd.type = CityCommand::Type::LIST;
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
    if (verb == "depositar" || verb == "retirar")
    {
        CityCommand::Type type = (verb == "depositar")
                                     ? CityCommand::Type::DEPOSIT
                                     : CityCommand::Type::WITHDRAW;
        std::string first;
        if (!(ss >> first))
            return std::nullopt;
        return parseGoldOrItem(type, first, ss);
    }

    return std::nullopt;
}
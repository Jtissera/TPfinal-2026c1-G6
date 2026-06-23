#include "ZoneNaming.h"

std::string ZoneNaming::toPoolPrefix(ZoneType zone) const
{
    switch (zone)
    {
    case ZoneType::COMBAT:
        return "combat";
    case ZoneType::CAVERN:
        return "cavern";
    case ZoneType::DUNGEON:
        return "dungeon";
    case ZoneType::SAFE:
        return "safe";
    case ZoneType::CITY:
        return "city";
    case ZoneType::DESERT:
        return "desert";
    case ZoneType::FOREST:
        return "forest";
    }
    return "";
}

#ifndef TALLER_TP_ZONENAMING_H
#define TALLER_TP_ZONENAMING_H

#pragma once
#include "editor/map/tile.h"
#include <string>

class ZoneNaming
{
public:
    // Convierte ZoneType a su nombre en minuscula, usado como
    // prefijo de las claves del TOML (ej: "combat_pool").
    static std::string toPoolPrefix(ZoneType zone)
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
};

#endif // TALLER_TP_ZONENAMING_H

#pragma once

#include "editor/map/tile.h"
#include <string>

class ZoneNaming
{
public:
    std::string toPoolPrefix(ZoneType zone) const;
};
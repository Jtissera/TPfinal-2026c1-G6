#pragma once
#include "../../editor/map/mapData.h"
#include <optional>
#include <utility>

class PriestLocator
{
public:
    static std::optional<std::pair<int, int>>
    findNearest(const MapData &map, int fromX, int fromY);
};
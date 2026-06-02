#include "priestLocator.h"
#include "../../editor/map/tile.h"
#include "common/npcType.h"
#include <cmath>
#include <limits>

std::optional<std::pair<int, int>>
PriestLocator::findNearest(const MapData &map, int fromX, int fromY)
{
    std::optional<std::pair<int, int>> best;
    float bestDist = std::numeric_limits<float>::max();

    for (uint16_t y = 0; y < map.height(); ++y)
    {
        for (uint16_t x = 0; x < map.width(); ++x)
        {
            if (map.at(x, y).npc != NpcType::PRIEST)
                continue;
            float dx = static_cast<float>(x - fromX);
            float dy = static_cast<float>(y - fromY);
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < bestDist)
            {
                bestDist = dist;
                best = {x, y};
            }
        }
    }
    return best;
}
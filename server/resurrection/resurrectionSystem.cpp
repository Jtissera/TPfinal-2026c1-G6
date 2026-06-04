#include "resurrectionSystem.h"
#include <vector>

void ResurrectionSystem::enqueue(uint32_t playerId,
                                 int targetX, int targetY,
                                 float delayMs)
{
    pending[playerId] = {targetX, targetY, delayMs};
}

bool ResurrectionSystem::isPending(uint32_t playerId) const
{
    return pending.count(playerId) > 0;
}

void ResurrectionSystem::tick(float deltaMs,
                              const ResurrectCallback &onComplete)
{
    std::vector<uint32_t> completed;

    for (auto &[id, res] : pending)
    {
        res.remainingMs -= deltaMs;
        if (res.remainingMs <= 0.0f)
            completed.push_back(id);
    }

    for (uint32_t id : completed)
    {
        auto res = pending.at(id);
        pending.erase(id);
        onComplete(id, res.targetTileX, res.targetTileY);
    }
}

void ResurrectionSystem::cancel(uint32_t playerId)
{
    pending.erase(playerId);
}
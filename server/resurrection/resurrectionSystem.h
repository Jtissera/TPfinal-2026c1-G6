#pragma once
#include <cstdint>
#include <unordered_map>
#include <functional>

struct PendingResurrection
{
    int targetTileX;
    int targetTileY;
    float remainingMs;
};

using ResurrectCallback = std::function<void(uint32_t, int, int)>;

class ResurrectionSystem
{
public:
    void enqueue(uint32_t playerId, int targetX, int targetY, float delayMs);

    bool isPending(uint32_t playerId) const;

    void tick(float deltaMs, const ResurrectCallback &onComplete);

    void cancel(uint32_t playerId);

private:
    std::unordered_map<uint32_t, PendingResurrection> pending;
};
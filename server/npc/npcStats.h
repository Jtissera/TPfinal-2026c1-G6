#pragma once
#include "common/npcType.h"
#include "editor/map/tile.h"
#include <cstdint>
#include <string>
#include <vector>

struct NpcStats
{
    NpcType type = NpcType::NONE;
    std::string name;
    std::string typeName;

    int16_t maxHp;
    uint16_t damageMin;
    uint16_t damageMax;
    uint8_t level;
    uint8_t agility;
    uint8_t strength;
    int detectionRange;
    int homeRange;
    uint32_t attackCooldownMs;
    uint32_t moveCooldownMs;

    std::vector<std::string> zones;
    bool hostile = true;

    ZoneType homeZone = ZoneType::COMBAT;

    float goldMultiplier = 1.0f;
    float xpMultiplier = 1.0f;
    float itemMultiplier = 1.0f;
};
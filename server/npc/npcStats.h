#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "editor/map/tile.h" // ZoneType

struct NpcStats
{
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

    // Zona en la que vive este NPC — determina confinamiento y multiplicadores
    ZoneType homeZone = ZoneType::COMBAT;

    // Multiplicadores de drop respecto a los valores base del config
    float goldMultiplier = 1.0f;
    float xpMultiplier = 1.0f;
    float itemMultiplier = 1.0f;
};
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct NpcStats {
    std::string typeName;
    int16_t  maxHp;
    uint16_t damageMin;
    uint16_t damageMax;
    uint8_t  level;
    uint8_t  agility;
    uint8_t  strength;
    int      detectionRange;
    int      homeRange;
    uint32_t attackCooldownMs;
    uint32_t moveCooldownMs;
    std::vector<std::string> zones;
    // Indica si este NPC es una criatura agresiva.
    // true  = esqueleto, goblin, zombie, etc.
    // false = comerciante, sacerdote, banquero, etc.
    bool hostile = true;

};
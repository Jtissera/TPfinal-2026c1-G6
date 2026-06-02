#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct NpcMoveIntent
{
    uint32_t npcId;
    int fromX, fromY;
    int toX, toY;
};

struct NpcAttack
{
    uint32_t targetPlayerId;
    int16_t damage;
    float xpMultiplier = 1.0f;
};

struct NpcDeathResult
{
    uint32_t npcId;
    int tileX;
    int tileY;
    uint32_t goldDrop;
    std::string itemDrop; // vacío si no hay drop
};

struct NpcTickResult
{
    std::vector<NpcMoveIntent> moveIntents; // GameWorld aplica si puede
    std::vector<NpcAttack> attacks;
    std::vector<NpcDeathResult> deaths;
};

#pragma once
#include "server/game/items/item.h"
#include <cstdint>
#include <string>
#include <vector>

struct NpcMoveIntent
{
    uint32_t npcId;
    int fromX;
    int fromY;
    int toX;
    int toY;
};

struct NpcAttack
{
    uint32_t npcId;
    uint32_t targetPlayerId;
    int16_t damage;
    float xpMultiplier = 1.0f;
};

struct NpcDeathResult
{
    uint32_t npcId;
    uint32_t killerPlayerId;
    int tileX;
    int tileY;
    uint32_t goldDrop;
    std::string itemDrop;
};

struct NpcTickResult
{
    std::vector<NpcMoveIntent> moveIntents;
    std::vector<NpcAttack> attacks;
    std::vector<NpcDeathResult> deaths;
};

struct NpcDropResult
{
    bool hasGold = false;
    uint32_t goldInstanceId = 0;
    uint32_t goldAmount = 0;

    bool hasItem = false;
    Item droppedItem;

    int tileX = 0;
    int tileY = 0;
};
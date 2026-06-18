#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "server/game/items/item.h"

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
    uint32_t killerPlayerId;
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
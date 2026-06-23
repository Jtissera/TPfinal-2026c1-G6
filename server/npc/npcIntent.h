#pragma once
#include "npcState.h"
#include <cstdint>

struct NpcIntent
{
    enum class Type
    {
        IDLE,
        MOVE,
        ATTACK
    };

    Type type = Type::IDLE;
    int tileX = 0;
    int tileY = 0;
    uint32_t targetId = 0;
    NpcState nextState = NpcState::IDLE;
};
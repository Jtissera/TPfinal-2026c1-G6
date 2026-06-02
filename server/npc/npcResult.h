#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct NpcMoveIntent {
    uint32_t npcId;
    float fromX, fromY;   // posicion actual en pixeles
    float toX,   toY;     // posicion destino en pixeles
};

struct NpcAttackIntent {
    uint32_t npcId;
    uint32_t targetPlayerId;
    int16_t  damage;
};

struct NpcDeathResult {
    uint32_t npcId;
    float    pixelX, pixelY;
    // Para backward compat con GameLoop que usa tileX/Y al broadcast:
    int      tileX,  tileY;
    uint32_t goldDrop;
    std::string itemDrop;
};

struct NpcTickResult {
    std::vector<NpcMoveIntent>  moveIntents;
    std::vector<NpcAttackIntent> attacks;
    std::vector<NpcDeathResult>  deaths;
};
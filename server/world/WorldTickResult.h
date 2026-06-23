#pragma once

#include "DeathResult.h"
#include "../npc/npcResult.h"
#include "../../common/dtos/gameTypes.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

struct InstanceEntry
{
    uint32_t playerId;
    std::string targetMap;
    int returnTileX;
    int returnTileY;
};

struct NpcSpawnEvent
{
    uint32_t npcId;
    NpcType type;
    std::string name;
    uint16_t x;
    uint16_t y;
    uint16_t hp;
    uint16_t maxHp;
    uint16_t level;
    bool hostile;
};

struct WorldTickResult
{
    struct PlayerHit
    {
        uint32_t playerId;
        int16_t damage;
    };

    struct PlayerResurrection
    {
        uint32_t playerId;
        uint16_t tileX;
        uint16_t tileY;
    };

    struct ResurrectStartedInfo
    {
        uint32_t playerId;
        uint32_t delayMs;
    };

    struct ClanAllyHit
    {
        std::string clanName;
        std::string targetName;
        uint32_t targetId;
    };

    struct NpcAttackAnim
    {
        uint32_t npcId;
        Direction direction;
    };

    std::vector<uint32_t> playersDied;
    std::vector<uint32_t> playersChanged;
    std::vector<uint32_t> npcsMoved;
    std::vector<NpcDeathResult> npcDeaths;
    std::vector<PlayerHit> playerHits;
    std::vector<InstanceEntry> instanceTransitions;
    std::vector<NpcSpawnEvent> spawnedNpcs;
    std::vector<std::pair<uint32_t, DeathResult>> playerDeathsByNpc;
    std::vector<PlayerResurrection> playersResurrected;
    std::vector<ResurrectStartedInfo> resurrectionStarted;
    std::vector<ClanAllyHit> clanAllyHits;
    std::vector<NpcAttackAnim> npcAttacksForAnim;
};
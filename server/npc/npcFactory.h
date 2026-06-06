#pragma once
#include "npc.h"
#include "npcRepository.h"

class NpcFactory {
public:
    explicit NpcFactory(const NpcRepository& repo);
    Npc create(const std::string& typeName, int tileX, int tileY);

private:
    const NpcRepository& repo;
    static constexpr uint32_t FIRST_NPC_ID = 10000;
    uint32_t nextId = FIRST_NPC_ID;
};
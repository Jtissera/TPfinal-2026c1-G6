#pragma once
#include "npc.h"
#include "npcRepository.h"

class NpcFactory {
public:
    explicit NpcFactory(const NpcRepository& repo);
    Npc create(const std::string& typeName, int tileX, int tileY);

private:
    const NpcRepository& repo;
    uint32_t nextId = 1;
};
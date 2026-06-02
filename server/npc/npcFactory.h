#pragma once
#include "npc.h"
#include "npcRepository.h"

class NpcFactory {
public:
    explicit NpcFactory(const NpcRepository& repo);
    Npc create(uint32_t id,const std::string& typeName, int tileX, int tileY, int tileSize);

private:
    const NpcRepository& repo;
    //uint32_t nextId = 1;
};
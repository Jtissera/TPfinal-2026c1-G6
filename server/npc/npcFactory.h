#pragma once
#include "npc.h"
#include "npcRepository.h"
#include <iostream>
#include <toml++/toml.hpp>

class NpcFactory
{
public:
    NpcFactory(const NpcRepository &repo, const toml::table &config);

    Npc create(const std::string &typeName, int tileX, int tileY);

private:
    const NpcRepository &repo;
    uint32_t nextId;
};

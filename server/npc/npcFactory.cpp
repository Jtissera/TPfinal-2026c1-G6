#include "npcFactory.h"

#include <iostream>

NpcFactory::NpcFactory(const NpcRepository& repo)
    : repo(repo) {}

Npc NpcFactory::create(const std::string& typeName, int tileX, int tileY) {
    NpcStats stats = repo.get(typeName);

    std::cout << "[NPC FACTORY] create id="
              << nextId
              << " typeName="
              << stats.typeName
              << " name="
              << stats.name
              << " type="
              << static_cast<int>(stats.type)
              << " tile=("
              << tileX
              << ", "
              << tileY
              << ")"
              << std::endl;

    return Npc(nextId++, stats, tileX, tileY);
}
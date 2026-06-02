#include "npcFactory.h"

NpcFactory::NpcFactory(const NpcRepository& repo)
    : repo(repo) {}

Npc NpcFactory::create(const std::string& typeName, int tileX, int tileY) {
    return Npc(nextId++, repo.get(typeName), 
               npcTypeFromKey(typeName), tileX, tileY);
}
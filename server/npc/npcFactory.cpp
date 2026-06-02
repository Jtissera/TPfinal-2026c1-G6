#include "npcFactory.h"

NpcFactory::NpcFactory(const NpcRepository& repo)
    : repo(repo) {}

    //sacar del toml ese 96
Npc NpcFactory::create(uint32_t id,const std::string& typeName, int tileX, int tileY, int tileSize) {
    return Npc(id, repo.get(typeName), 
               npcTypeFromKey(typeName), tileX, tileY, 96);
}
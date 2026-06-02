#include "npcFactory.h"

NpcFactory::NpcFactory(const NpcRepository& repo)
    : repo(repo) {}

Npc NpcFactory::create(uint32_t id, const std::string& typeName,
                        float spawnPixelX, float spawnPixelY, int tileSize) {
    return Npc(id, repo.get(typeName), npcTypeFromKey(typeName),
               spawnPixelX, spawnPixelY, tileSize);  // pasar tileSize real, no 96 hardcodeado
}
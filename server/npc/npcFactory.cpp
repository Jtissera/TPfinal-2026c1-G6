#include "npcFactory.h"

NpcFactory::NpcFactory(const NpcRepository &repo, const toml::table &config)
    : repo(repo),
      nextId(config["npc_factory"]["first_npc_id"].value_or<uint32_t>(10000))
{
}

Npc NpcFactory::create(const std::string &typeName, int tileX, int tileY)
{
    NpcStats stats = repo.get(typeName);

    std::cout << "[NPC FACTORY] create id=" << nextId
              << " typeName=" << stats.typeName
              << " name=" << stats.name
              << " type=" << static_cast<int>(stats.type)
              << " tile=(" << tileX << ", " << tileY << ")"
              << std::endl;

    return Npc(nextId++, stats, tileX, tileY);
}
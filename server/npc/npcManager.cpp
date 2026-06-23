#include "npcManager.h"

NpcManager::NpcManager(NpcFactory &factory,
                       const CollisionSystem &collision,
                       const MapData &mapData,
                       const toml::table &config)
    : factory(factory),
      collision(collision),
      mapData(mapData),
      ai(config),
      baseGoldChance(config["npc_drops"]["base_gold_chance"].value_or(8.0f)),
      baseItemChance(config["npc_drops"]["base_item_chance"].value_or(1.0f)),
      goldFactorMin(config["npc_drops"]["gold_factor_min"].value_or(0.01f)),
      goldFactorRange(config["npc_drops"]["gold_factor_range"].value_or(0.19f))
{
}

uint32_t NpcManager::spawnNpc(const std::string &typeName, int tileX, int tileY)
{
    Npc npc = factory.create(typeName, tileX, tileY);
    uint32_t id = npc.getId();
    npcs.emplace(id, std::move(npc));
    return id;
}

void NpcManager::removeNpc(uint32_t npcId)
{
    npcs.erase(npcId);
}

void NpcManager::applyMove(uint32_t npcId, int toX, int toY)
{
    std::unordered_map<uint32_t, Npc>::iterator it = npcs.find(npcId);
    if (it != npcs.end())
    {
        it->second.setTilePos(toX, toY);
        it->second.resetMoveCooldown();
    }
}

NpcTickResult NpcManager::tick(const std::unordered_map<uint32_t, Player> &players)
{
    NpcTickResult result;

    for (std::pair<const uint32_t, Npc> &entry : npcs)
    {
        Npc &npc = entry.second;

        if (!npc.isAlive())
        {
            continue;
        }

        if (!npc.isHostile())
        {
            continue;
        }

        NpcIntent intent = ai.decide(npc, players);

        npc.setState(intent.nextState);
        npc.setTargetId(intent.targetId);

        if (intent.type == NpcIntent::Type::MOVE && npc.canMove())
        {
            result.moveIntents.push_back(
                {entry.first, npc.getTileX(), npc.getTileY(),
                 intent.tileX, intent.tileY});
        }
        else if (intent.type == NpcIntent::Type::ATTACK && npc.canAttack())
        {
            result.attacks.push_back({entry.first,
                                      intent.targetId,
                                      rollDamage(npc.getStats()),
                                      npc.getStats().xpMultiplier});
            npc.resetAttackCooldown();
        }
    }

    return result;
}

const std::unordered_map<uint32_t, Npc> &NpcManager::getNpcs() const
{
    return npcs;
}

int NpcManager::count() const
{
    return static_cast<int>(npcs.size());
}

bool NpcManager::isSameZone(int tileX, int tileY, ZoneType zone) const
{
    if (!collision.isInBounds(tileX, tileY))
    {
        return false;
    }
    return mapData.at(tileX, tileY).zone == zone;
}

bool NpcManager::hasNpc(uint32_t npcId) const
{
    return npcs.find(npcId) != npcs.end();
}

bool NpcManager::damageNpc(uint32_t npcId, int16_t damage, uint32_t attackerPlayerId)
{
    std::unordered_map<uint32_t, Npc>::iterator it = npcs.find(npcId);

    if (it == npcs.end())
    {
        return false;
    }

    Npc &npc = it->second;

    if (!npc.isHostile())
    {
        return false;
    }

    if (!npc.isAlive())
    {
        return false;
    }

    npc.takeDamage(damage);

    if (npc.isAlive())
    {
        npc.setTargetId(attackerPlayerId);
        npc.setState(NpcState::CHASING);
    }

    return true;
}

Npc &NpcManager::getNpc(uint32_t npcId)
{
    std::unordered_map<uint32_t, Npc>::iterator it = npcs.find(npcId);
    if (it == npcs.end())
    {
        throw std::runtime_error("NpcManager::getNpc: NPC not found");
    }
    return it->second;
}

const Npc &NpcManager::getNpc(uint32_t npcId) const
{
    std::unordered_map<uint32_t, Npc>::const_iterator it = npcs.find(npcId);
    if (it == npcs.end())
    {
        throw std::runtime_error("NpcManager::getNpc const: NPC not found");
    }
    return it->second;
}

Npc *NpcManager::findNpc(uint32_t npcId)
{
    std::unordered_map<uint32_t, Npc>::iterator it = npcs.find(npcId);
    if (it == npcs.end())
    {
        return nullptr;
    }
    return &it->second;
}

const Npc *NpcManager::findNpc(uint32_t npcId) const
{
    std::unordered_map<uint32_t, Npc>::const_iterator it = npcs.find(npcId);
    if (it == npcs.end())
    {
        return nullptr;
    }
    return &it->second;
}

void NpcManager::startRespawn(uint32_t npcId, float respawnMs)
{
    std::unordered_map<uint32_t, Npc>::iterator it = npcs.find(npcId);
    if (it == npcs.end())
    {
        std::cout << "[NPC MANAGER] startRespawn failed, NPC not found id="
                  << npcId << std::endl;
        return;
    }
    it->second.startRespawn(respawnMs);
}

std::vector<uint32_t> NpcManager::tickRespawns(float deltaMs)
{
    std::vector<uint32_t> readyToRespawn;

    for (std::pair<const uint32_t, Npc> &entry : npcs)
    {
        Npc &npc = entry.second;
        if (!npc.isRespawning())
        {
            continue;
        }
        if (npc.tickRespawn(deltaMs))
        {
            readyToRespawn.push_back(entry.first);
        }
    }

    return readyToRespawn;
}

NpcDeathResult NpcManager::buildDeathResult(const Npc &npc, uint32_t killerPlayerId) const
{
    NpcDeathResult d{};

    d.npcId = npc.getId();
    d.killerPlayerId = killerPlayerId;
    d.tileX = npc.getTileX();
    d.tileY = npc.getTileY();
    d.goldDrop = 0;
    d.itemDrop = "";

    const NpcStats &stats = npc.getStats();
    int roll = std::rand() % 100;

    float effectiveGoldChance = baseGoldChance * stats.goldMultiplier;
    float effectiveItemChance = baseItemChance * stats.itemMultiplier;

    if (roll < static_cast<int>(effectiveGoldChance))
    {
        float factor = goldFactorMin +
                       (static_cast<float>(std::rand() % 100) / 100.0f) * goldFactorRange;
        d.goldDrop = static_cast<uint32_t>(factor * npc.getMaxHp() * stats.goldMultiplier);
    }
    else if (roll < static_cast<int>(effectiveGoldChance + effectiveItemChance))
    {
        d.itemDrop = (std::rand() % 2 == 0) ? "health_potion" : "mana_potion";
    }
    else if (roll < static_cast<int>(effectiveGoldChance + effectiveItemChance * 2))
    {
        d.itemDrop = "sword";
    }

    return d;
}

int16_t NpcManager::rollDamage(const NpcStats &stats) const
{
    int range = stats.damageMax - stats.damageMin;
    return static_cast<int16_t>(stats.damageMin + (range > 0 ? std::rand() % range : 0));
}
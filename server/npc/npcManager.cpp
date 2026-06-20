#include "npcManager.h"
#include <cstdlib>
#include <iostream>

NpcManager::NpcManager(NpcFactory &factory, const CollisionSystem &collision,
                       const MapData &mapData)
    : factory(factory), collision(collision), mapData(mapData) {}

uint32_t NpcManager::spawnNpc(const std::string &typeName, int tileX,
                              int tileY)
{
  Npc npc = factory.create(typeName, tileX, tileY);
  uint32_t id = npc.getId();
  npcs.emplace(id, std::move(npc));
  return id;
}

void NpcManager::removeNpc(uint32_t npcId) {
    npcs.erase(npcId);
}

void NpcManager::applyMove(uint32_t npcId, int toX, int toY)
{
  auto it = npcs.find(npcId);
  if (it != npcs.end())
  {
    it->second.setTilePos(toX, toY);
    it->second.resetMoveCooldown();
  }
}

NpcTickResult
NpcManager::tick(const std::unordered_map<uint32_t, Player> &players)
{
    NpcTickResult result;

    for (auto &[id, npc] : npcs)
    {
        // Si el NPC está muerto o respawneando, no ataca.
        if (!npc.isAlive()) {
            continue;
        }

        // Los NPCs no hostiles no persiguen ni atacan.
        if (!npc.isHostile()) {
            continue;
        }

        // La IA decide qué quiere hacer este NPC.
        NpcIntent intent = ai.decide(npc, players);

        // Guardamos el estado visual/lógico de IA.
        npc.setState(intent.nextState);
        npc.setTargetId(intent.targetId);

        // Si quiere moverse y el cooldown se lo permite,
        // devolvemos una intención de movimiento para que GameWorld valide colisiones.
        if (intent.type == NpcIntent::Type::MOVE && npc.canMove())
        {
            result.moveIntents.push_back(
                {id, npc.getTileX(), npc.getTileY(), intent.tileX, intent.tileY});
        }
        // Si quiere atacar y el cooldown se lo permite,
        // devolvemos una intención de ataque para que GameWorld aplique daño.
        else if (intent.type == NpcIntent::Type::ATTACK && npc.canAttack())
        {
            result.attacks.push_back({
            id,                          
            intent.targetId,
            rollDamage(npc.getStats()),
            npc.getStats().xpMultiplier
            });

            npc.resetAttackCooldown();
        }
    }

    return result;
}

int16_t NpcManager::rollDamage(const NpcStats& stats) const {
    int range = stats.damageMax - stats.damageMin;
    return static_cast<int16_t>(stats.damageMin + (range > 0 ? std::rand() % range : 0));
}

NpcDeathResult NpcManager::buildDeathResult(const Npc &npc,const uint32_t killerPlayerId) const
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

  float effectiveGoldChance = 8.0f * stats.goldMultiplier;
  float effectiveItemChance = 1.0f * stats.itemMultiplier;

  if (roll < static_cast<int>(effectiveGoldChance))
  {
    float factor = 0.01f + (std::rand() % 100) / 100.0f * 0.19f;
    d.goldDrop = static_cast<uint32_t>(
        factor * npc.getMaxHp() * stats.goldMultiplier);
  }
  else if (roll < static_cast<int>(effectiveGoldChance + effectiveItemChance))
  {
    d.itemDrop = (std::rand() % 2 == 0) ? "pocion_vida" : "pocion_mana";
  }
  else if (roll < static_cast<int>(effectiveGoldChance + effectiveItemChance * 2))
  {
    d.itemDrop = "espada";
  }

  return d;
}

bool NpcManager::isSameZone(int tileX, int tileY, ZoneType zone) const
{
  if (!collision.isInBounds(tileX, tileY))
    return false;
  return mapData.at(tileX, tileY).zone == zone;
}
bool NpcManager::damageNpc(
    uint32_t npcId,
    int16_t damage,
    uint32_t attackerPlayerId
) {
    auto it = npcs.find(npcId);

    if (it == npcs.end()) {
        return false;
    }

    Npc& npc = it->second;

    // Comerciante, sacerdote, banquero, etc. no son atacables.
    if (!npc.isHostile()) {
        return false;
    }

    if (!npc.isAlive()) {
        return false;
    }

    npc.takeDamage(damage);

    if (npc.isAlive()) {
        npc.setTargetId(attackerPlayerId);
        npc.setState(NpcState::CHASING);
    }

    return true;
}

bool NpcManager::hasNpc(uint32_t npcId) const {
    return npcs.find(npcId) != npcs.end();
}

Npc* NpcManager::findNpc(uint32_t npcId) {
    auto it = npcs.find(npcId);

    if (it == npcs.end()) {
        return nullptr;
    }

    return &it->second;
}

const Npc* NpcManager::findNpc(uint32_t npcId) const {
    auto it = npcs.find(npcId);

    if (it == npcs.end()) {
        return nullptr;
    }

    return &it->second;
}

Npc& NpcManager::getNpc(uint32_t npcId) {
    auto it = npcs.find(npcId);

    if (it == npcs.end()) {
        throw std::runtime_error("NpcManager::getNpc: NPC inexistente");
    }

    return it->second;
}

const Npc& NpcManager::getNpc(uint32_t npcId) const {
    auto it = npcs.find(npcId);

    if (it == npcs.end()) {
        throw std::runtime_error("NpcManager::getNpc const: NPC inexistente");
    }

    return it->second;
}

void NpcManager::startRespawn(uint32_t npcId, float respawnMs) {
    auto it = npcs.find(npcId);

    if (it == npcs.end()) {
        std::cout << "[NPC MANAGER] startRespawn falló, npc inexistente id="
                  << npcId
                  << std::endl;
        return;
    }

    it->second.startRespawn(respawnMs);
}

std::vector<uint32_t> NpcManager::tickRespawns(float deltaMs) {
    std::vector<uint32_t> readyToRespawn;

    for (auto& [npcId, npc] : npcs) {
        if (!npc.isRespawning()) {
            continue;
        }

        if (npc.tickRespawn(deltaMs)) {
            readyToRespawn.push_back(npcId);
        }
    }

    return readyToRespawn;
}
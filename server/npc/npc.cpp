#include "npc.h"

Npc::Npc(uint32_t id, const NpcStats &stats, int spawnTileX, int spawnTileY)
    : id(id),
      stats(stats),
      tileX(spawnTileX),
      tileY(spawnTileY),
      spawnTileX(spawnTileX),
      spawnTileY(spawnTileY),
      hp(stats.maxHp),
      state(NpcState::IDLE),
      lifeState(NpcLifeState::ALIVE),
      respawnRemainingMs(0.0f),
      targetId(0),
      lastAttack(Clock::now()),
      lastMove(Clock::now()),
      hasAttacked(false)
{
}

uint32_t Npc::getId() const { return id; }
int Npc::getTileX() const { return tileX; }
int Npc::getTileY() const { return tileY; }
int16_t Npc::getHp() const { return hp; }
int16_t Npc::getMaxHp() const { return stats.maxHp; }
uint8_t Npc::getLevel() const { return stats.level; }
uint8_t Npc::getAgility() const { return stats.agility; }
uint8_t Npc::getStrength() const { return stats.strength; }
int Npc::getAttackRange() const { return 1; }

bool Npc::isAlive() const
{
    return lifeState == NpcLifeState::ALIVE && hp > 0;
}

uint16_t Npc::getWeaponDamageMin() const { return stats.damageMin; }
uint16_t Npc::getWeaponDamageMax() const { return stats.damageMax; }
uint16_t Npc::getArmorDefenseMin() const { return 0; }
uint16_t Npc::getArmorDefenseMax() const { return 0; }
uint16_t Npc::getHelmetDefenseMin() const { return 0; }
uint16_t Npc::getHelmetDefenseMax() const { return 0; }
uint16_t Npc::getShieldDefenseMin() const { return 0; }
uint16_t Npc::getShieldDefenseMax() const { return 0; }

void Npc::takeDamage(int16_t dmg)
{
    if (lifeState != NpcLifeState::ALIVE)
    {
        return;
    }
    hp = std::max<int16_t>(0, static_cast<int16_t>(hp - dmg));
}

NpcType Npc::getType() const { return stats.type; }
const std::string &Npc::getName() const { return stats.name; }
const std::string &Npc::getTypeName() const { return stats.typeName; }
int Npc::getSpawnTileX() const { return spawnTileX; }
int Npc::getSpawnTileY() const { return spawnTileY; }
int Npc::getDetectionRange() const { return stats.detectionRange; }
int Npc::getHomeRange() const { return stats.homeRange; }
const NpcStats &Npc::getStats() const { return stats; }
NpcState Npc::getState() const { return state; }
uint32_t Npc::getTargetId() const { return targetId; }
bool Npc::isHostile() const { return stats.hostile; }

void Npc::setTilePos(int tx, int ty)
{
    tileX = tx;
    tileY = ty;
}
void Npc::setState(NpcState s) { state = s; }
void Npc::setTargetId(uint32_t id) { targetId = id; }

void Npc::clearTarget()
{
    targetId = 0;
    state = NpcState::IDLE;
}

bool Npc::canAttack() const
{
    // Si el NPC nunca atacó, puede atacar inmediatamente.
    // El cooldown empieza a contar recién después del primer ataque.
    if (!hasAttacked)
    {
        return true;
    }

    std::chrono::milliseconds elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - lastAttack);

    return elapsed.count() >= stats.attackCooldownMs;
}

bool Npc::canMove() const
{
    std::chrono::milliseconds elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - lastMove);
    return elapsed.count() >= stats.moveCooldownMs;
}

void Npc::resetAttackCooldown() { hasAttacked =true; lastAttack = Clock::now(); }
void Npc::resetMoveCooldown() { lastMove = Clock::now(); }

bool Npc::isRespawning() const
{
    return lifeState == NpcLifeState::RESPAWNING;
}

NpcLifeState Npc::getLifeState() const
{
    return lifeState;
}

void Npc::startRespawn(float respawnMs)
{
    hp = 0;
    lifeState = NpcLifeState::RESPAWNING;
    state = NpcState::IDLE;
    targetId = 0;
    respawnRemainingMs = respawnMs;
}

bool Npc::tickRespawn(float deltaMs)
{
    if (lifeState != NpcLifeState::RESPAWNING)
    {
        return false;
    }
    respawnRemainingMs -= deltaMs;
    return respawnRemainingMs <= 0.0f;
}

void Npc::respawn()
{
    tileX = spawnTileX;
    tileY = spawnTileY;
    hp = stats.maxHp;
    lifeState = NpcLifeState::ALIVE;
    state = NpcState::IDLE;
    targetId = 0;
    respawnRemainingMs = 0.0f;
}
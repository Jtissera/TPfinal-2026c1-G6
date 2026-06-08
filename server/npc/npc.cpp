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
    lastMove(Clock::now())
{
}

void Npc::takeDamage(int16_t dmg) {
    // No puede recibir daño si no está activo.
    if (lifeState != NpcLifeState::ALIVE) {
        return;
    }

    hp = std::max<int16_t>(0, static_cast<int16_t>(hp - dmg));
}

bool Npc::canAttack() const {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - lastAttack).count();
    return elapsed >= stats.attackCooldownMs;
}

bool Npc::canMove() const {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - lastMove).count();
    return elapsed >= stats.moveCooldownMs;
}

void Npc::resetAttackCooldown() { lastAttack = Clock::now(); }
void Npc::resetMoveCooldown()   { lastMove   = Clock::now(); }


bool Npc::isRespawning() const {
    return lifeState == NpcLifeState::RESPAWNING;
}

NpcLifeState Npc::getLifeState() const {
    return lifeState;
}

void Npc::startRespawn(float respawnMs) {
    // El NPC queda muerto, pero sigue existiendo en el NpcManager.
    hp = 0;

    // Pasa a estado de respawn.
    lifeState = NpcLifeState::RESPAWNING;

    // Ya no persigue ni ataca.
    state = NpcState::IDLE;
    targetId = 0;

    // Tiempo restante hasta reaparecer.
    respawnRemainingMs = respawnMs;
}

bool Npc::tickRespawn(float deltaMs) {
    // Si no está respawneando, no hay nada que hacer.
    if (lifeState != NpcLifeState::RESPAWNING) {
        return false;
    }

    // Restamos tiempo.
    respawnRemainingMs -= deltaMs;

    // Devuelve true cuando ya puede revivir.
    return respawnRemainingMs <= 0.0f;
}

void Npc::respawn() {
    // Vuelve a su punto original.
    tileX = spawnTileX;
    tileY = spawnTileY;

    // Recupera toda la vida.
    hp = stats.maxHp;

    // Vuelve a estar activo.
    lifeState = NpcLifeState::ALIVE;

    // Arranca quieto.
    state = NpcState::IDLE;
    targetId = 0;

    respawnRemainingMs = 0.0f;
}
#include "npc.h"

Npc::Npc(uint32_t id, const NpcStats& stats, NpcType type,
         int spawnTileX, int spawnTileY)
    : id(id), stats(stats), type(type)
    , tileX(spawnTileX), tileY(spawnTileY)
    , spawnTileX(spawnTileX), spawnTileY(spawnTileY)
    , hp(stats.maxHp)
    , lastAttack(Clock::now() - std::chrono::milliseconds(stats.attackCooldownMs))
    , lastMove(Clock::now()   - std::chrono::milliseconds(stats.moveCooldownMs))
{}

void Npc::takeDamage(int16_t dmg) {
    hp = std::max<int16_t>(0, hp - dmg);
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
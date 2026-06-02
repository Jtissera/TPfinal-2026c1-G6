#include "npc.h"

Npc::Npc(uint32_t id, const NpcStats& stats, NpcType type,
         float spawnPixelX, float spawnPixelY, int tileSize)
    : id(id)
    , stats(stats)
    , type(type)
    , pixelX(spawnPixelX)
    , pixelY(spawnPixelY)
    , spawnPixelX(spawnPixelX)
    , spawnPixelY(spawnPixelY)
    , tileSize(tileSize)
    // Convertimos rangos de tiles a pixeles
    // detectionRange y homeRange estaban en tiles en npcStats
    , detectionRangePx(static_cast<float>(stats.detectionRange) * tileSize)
    , homeRangePx(static_cast<float>(stats.homeRange) * tileSize)
    // Ataque melee: 1.5 tiles de rango
    , attackRangePx(tileSize * 1.5f)
    // Paso: un tile por movimiento
    , stepPx(static_cast<float>(tileSize))
    , hp(stats.maxHp)
    , lastAttack(Clock::now() - std::chrono::milliseconds(stats.attackCooldownMs))
    , lastMove(Clock::now()   - std::chrono::milliseconds(stats.moveCooldownMs))
{}

int Npc::getTileX() const {
    return static_cast<int>(pixelX) / tileSize;
}

int Npc::getTileY() const {
    return static_cast<int>(pixelY) / tileSize;
}

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
#pragma once
#include "../game/player/combatant.h"
#include "../../common/npcType.h"
#include "npcState.h"
#include "npcStats.h"
#include <algorithm>
#include <chrono>
#include <cstdint>

class Npc : public Combatant {
public:
    Npc(uint32_t id, const NpcStats& stats, NpcType type,
        float spawnPixelX, float spawnPixelY, int tileSize);

    // --- Combatant ---
    uint32_t getId()       const override { return id; }
    float    getPixelX()   const override { return pixelX; }
    float    getPixelY()   const override { return pixelY; }
    int      getTileX()    const override;
    int      getTileY()    const override;
    int16_t  getHp()       const override { return hp; }
    int16_t  getMaxHp()    const override { return stats.maxHp; }
    uint8_t  getLevel()    const override { return stats.level; }
    uint8_t  getAgility()  const override { return stats.agility; }
    uint8_t  getStrength() const override { return stats.strength; }
    bool     isAlive()     const override { return hp > 0; }

    // Rango de ataque en pixeles (melee: ~1.5 tiles)
    float    getAttackRangePx() const override { return attackRangePx; }

    uint16_t getWeaponDamageMin()  const override { return stats.damageMin; }
    uint16_t getWeaponDamageMax()  const override { return stats.damageMax; }
    uint16_t getArmorDefenseMin()  const override { return 0; }
    uint16_t getArmorDefenseMax()  const override { return 0; }
    uint16_t getHelmetDefenseMin() const override { return 0; }
    uint16_t getHelmetDefenseMax() const override { return 0; }
    uint16_t getShieldDefenseMin() const override { return 0; }
    uint16_t getShieldDefenseMax() const override { return 0; }

    void takeDamage(int16_t dmg) override;

    // --- Spawn / home ---
    float getSpawnPixelX()     const { return spawnPixelX; }
    float getSpawnPixelY()     const { return spawnPixelY; }

    // Rangos en pixeles (convertidos desde tiles en el constructor)
    float getDetectionRangePx()const { return detectionRangePx; }
    float getHomeRangePx()     const { return homeRangePx; }

    // Para GameLoop que manda pixeles al cliente
    float getStepPx()          const { return stepPx; }

    const NpcStats& getStats() const { return stats; }
    NpcState        getState() const { return state; }
    uint32_t        getTargetId() const { return targetId; }
    NpcType         getType()  const { return type; }

    void setPixelPos(float px, float py) { pixelX = px; pixelY = py; }
    void setState(NpcState s)            { state = s; }
    void setTargetId(uint32_t id)        { targetId = id; }
    void clearTarget()                   { targetId = 0; state = NpcState::IDLE; }

    bool canAttack() const;
    bool canMove()   const;
    void resetAttackCooldown();
    void resetMoveCooldown();

    Npc(const Npc&) = delete;
    Npc& operator=(const Npc&) = delete;
    Npc(Npc&&)      = default;
    Npc& operator=(Npc&&) = default;

private:
    uint32_t id;
    const NpcStats& stats;
    NpcType  type;

    float pixelX, pixelY;
    float spawnPixelX, spawnPixelY;

    int   tileSize;

    // Rangos en pixeles derivados de stats (en tiles) * tileSize
    float detectionRangePx;
    float homeRangePx;
    float attackRangePx;
    float stepPx;

    int16_t  hp;
    NpcState state    = NpcState::IDLE;
    uint32_t targetId = 0;

    using Clock = std::chrono::steady_clock;
    Clock::time_point lastAttack;
    Clock::time_point lastMove;
};
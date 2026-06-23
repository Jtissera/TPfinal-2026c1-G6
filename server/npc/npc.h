#pragma once
#include "../game/player/combatant.h"
#include "npcState.h"
#include "npcStats.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include "common/npcType.h"

class Npc : public Combatant
{
public:
  Npc(uint32_t id, const NpcStats &stats, int spawnTileX, int spawnTileY);

  Npc(const Npc &) = delete;
  Npc &operator=(const Npc &) = delete;
  Npc(Npc &&) = default;
  Npc &operator=(Npc &&) = default;

  uint32_t getId() const override;
  int getTileX() const override;
  int getTileY() const override;
  int16_t getHp() const override;
  int16_t getMaxHp() const override;
  uint8_t getLevel() const override;
  uint8_t getAgility() const override;
  uint8_t getStrength() const override;
  int getAttackRange() const override;
  bool isAlive() const override;

  uint16_t getWeaponDamageMin() const override;
  uint16_t getWeaponDamageMax() const override;
  uint16_t getArmorDefenseMin() const override;
  uint16_t getArmorDefenseMax() const override;
  uint16_t getHelmetDefenseMin() const override;
  uint16_t getHelmetDefenseMax() const override;
  uint16_t getShieldDefenseMin() const override;
  uint16_t getShieldDefenseMax() const override;

  void takeDamage(int16_t dmg) override;

  NpcType getType() const;
  const std::string &getName() const;
  const std::string &getTypeName() const;
  int getSpawnTileX() const;
  int getSpawnTileY() const;
  int getDetectionRange() const;
  int getHomeRange() const;
  const NpcStats &getStats() const;
  NpcState getState() const;
  uint32_t getTargetId() const;
  bool isHostile() const;

  void setTilePos(int tx, int ty);
  void setState(NpcState s);
  void setTargetId(uint32_t id);
  void clearTarget();

  bool canAttack() const;
  bool canMove() const;
  void resetAttackCooldown();
  void resetMoveCooldown();

  bool isRespawning() const;
  NpcLifeState getLifeState() const;
  void startRespawn(float respawnMs);
  bool tickRespawn(float deltaMs);
  void respawn();

private:
  uint32_t id;
  NpcStats stats;
  int tileX;
  int tileY;
  int spawnTileX;
  int spawnTileY;
  int16_t hp;
  NpcState state;
  NpcLifeState lifeState;
  float respawnRemainingMs;
  uint32_t targetId;

  using Clock = std::chrono::steady_clock;
  Clock::time_point lastAttack;
  Clock::time_point lastMove;
};
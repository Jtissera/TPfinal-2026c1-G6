#pragma once
#include "../game/player/combatant.h"
#include "npcState.h"
#include "npcStats.h"
#include <algorithm>
#include <chrono>
#include <cstdint>

#include "common/npcType.h"

class Npc : public Combatant {
public:
  Npc(uint32_t id, const NpcStats &stats, int spawnTileX, int spawnTileY);


    uint32_t getId()          const override { return id;              }
    int      getTileX()       const override { return tileX;           }
    int      getTileY()       const override { return tileY;           }
    int16_t  getHp()          const override { return hp;              }
    int16_t  getMaxHp()       const override { return stats.maxHp;     }
    uint8_t  getLevel()       const override { return stats.level;     }
    uint8_t  getAgility()     const override { return stats.agility;   }
    uint8_t  getStrength()    const override { return stats.strength;  }
    int      getAttackRange() const override { return 1;               }
    bool     isAlive()        const override { return hp > 0;          }
    NpcType getType() const {return stats.type;}
    const std::string& getName() const {return stats.name;}
    const std::string& getTypeName() const {return stats.typeName;}

    uint16_t getWeaponDamageMin()  const override { return stats.damageMin; }
    uint16_t getWeaponDamageMax()  const override { return stats.damageMax; }
    uint16_t getArmorDefenseMin()  const override { return 0; }
    uint16_t getArmorDefenseMax()  const override { return 0; }
    uint16_t getHelmetDefenseMin() const override { return 0; }
    uint16_t getHelmetDefenseMax() const override { return 0; }
    uint16_t getShieldDefenseMin() const override { return 0; }
    uint16_t getShieldDefenseMax() const override { return 0; }

  void takeDamage(int16_t dmg) override;

    int      getSpawnTileX()     const { return spawnTileX;           }
    int      getSpawnTileY()     const { return spawnTileY;           }
    int      getDetectionRange() const { return stats.detectionRange; }
    int      getHomeRange()      const { return stats.homeRange;      }
    const NpcStats& getStats()   const { return stats;                }
    NpcState getState()          const { return state;                }
    uint32_t getTargetId()       const { return targetId;             }
    bool isHostile() const { return stats.hostile; }


    void setTilePos(int tx, int ty) { tileX = tx; tileY = ty; }
    void setState(NpcState s)       { state = s;               }
    void setTargetId(uint32_t id)   { targetId = id;           }
    void clearTarget() { targetId = 0; state = NpcState::IDLE; }

  bool canAttack() const;
  bool canMove() const;
  void resetAttackCooldown();
  void resetMoveCooldown();

    Npc(const Npc&)            = delete;
    Npc& operator=(const Npc&) = delete;
    Npc(Npc&&)                 = default;
    Npc& operator=(Npc&&)      = default;

private:
  uint32_t id;
  const NpcStats &stats;
  int tileX, tileY;
  int spawnTileX, spawnTileY;
  int16_t hp;
  NpcState state = NpcState::IDLE;
  uint32_t targetId = 0;

    using Clock = std::chrono::steady_clock;
    Clock::time_point lastAttack;
    Clock::time_point lastMove;
};

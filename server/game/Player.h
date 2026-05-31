#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../common/dtos/gameTypes.h"
#include "inventory.h"
#include "raceRepository.h"
#include "classRepository.h"
#include "playerState.h"
#include "combatant.h"

class Player : public Combatant{
public:
    Player(uint32_t clientId,
           std::string name,
           const RaceStats&  race,
           const ClassStats& cls,
           int16_t maxHp,
           int16_t maxMana);

    bool isAlive()      const override;
    bool isGhost()      const;
    bool isMeditating() const;

    void takeDamage(int16_t dmg) override;
    void heal(int16_t amount);
    void restoreMana(int16_t amount);
    bool spendMana(int16_t cost);
    void addGold(uint32_t amount);

    // Devuelve oro en exceso al morir
    uint32_t die(uint32_t safeGold);
    void resurrect(int tileX, int tileY);

    //las fórmulas vienen de afuera
    void addExperience(uint32_t exp, uint32_t expLimit,
                       int16_t newMaxHp, int16_t newMaxMana);
    bool checkAndClearLevelUp();
    void levelUp(int16_t newMaxHp, int16_t newMaxMana);

    void startMeditating();
    void stopMeditating();

    void tick(float hpGained, float manaGained);

    std::vector<Item> purgeInventoryOnDeath();

    int getTileX() const override { return tileX; }
    int getTileY() const override{ return tileY; }
    void setTilePos(int tx, int ty) { tileX = tx; tileY = ty; }

    uint32_t getId()      const override{ return clientId; }  //mandar esto al cpp
    uint32_t getClientId()const { return clientId; }
    uint8_t  getLevel()   const override{ return level;    }
    int16_t  getHp()      const override{ return hp;       }
    int16_t  getMaxHp()   const override{ return maxHp;    }
    int16_t  getMana()    const { return mana;      }
    int16_t  getMaxMana() const { return maxMana;   }
    uint32_t getExp()     const { return experience;}
    uint32_t getGold()    const { return gold;      }
    uint8_t  getAgility()     const override { return race.agility;      }
    uint8_t  getStrength()    const override { return race.strength;     }
    int      getAttackRange() const override;  // en .cpp, depende del arma

    uint16_t getWeaponDamageMin()  const override;
    uint16_t getWeaponDamageMax()  const override;
    uint16_t getArmorDefenseMin()  const override;
    uint16_t getArmorDefenseMax()  const override;
    uint16_t getHelmetDefenseMin() const override;
    uint16_t getHelmetDefenseMax() const override;
    uint16_t getShieldDefenseMin() const override;
    uint16_t getShieldDefenseMax() const override;

    const RaceStats&  getRace() const { return race; }
    const ClassStats& getCls()  const { return cls;  }

    Inventory&       getInventory()       { return inventory; }
    const Inventory& getInventory() const { return inventory; }

    Player(const Player&)            = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&)                 = default;
    Player& operator=(Player&&)      = default;

private:
    uint32_t clientId;
    std::string name;
    const RaceStats&  race;
    const ClassStats& cls;

    int tileX = 0;
    int tileY = 0;

    uint8_t  level      = 1;
    int16_t  hp         = 0;
    int16_t  maxHp      = 0;
    int16_t  mana       = 0;
    int16_t  maxMana    = 0;
    uint32_t gold       = 0;
    uint32_t experience = 0;

    PlayerState state    = PlayerState::ALIVE;
    bool        didLevelUp = false;

    Inventory inventory;
};
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../common/dtos/gameTypes.h"
#include "items/item.h"
#include "player/combatant.h"
#include "player/inventory.h"
#include "player/playerState.h"
#include "stats/classStats.h"
#include "stats/raceStats.h"

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

    int getTileX() const override { return static_cast<int>(pixelX) / TILE_SIZE; }
    int getTileY() const override{ return static_cast<int>(pixelY)/ TILE_SIZE; }
    void setTilePos(int tx, int ty);
    std::string getName()const;
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

        // Devuelve la posición X real en píxeles.
    float getPixelX() const;

    // Devuelve la posición Y real en píxeles.
    float getPixelY() const;

    // Setea la posición real del jugador en píxeles.
    void setPixelPos(float x, float y);


    Player(const Player&)            = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&)                 = default;
    Player& operator=(Player&&)      = default;

private:
    uint32_t clientId;
    std::string name;
    const RaceStats&  race;
    const ClassStats& cls;
    const int TILE_SIZE = 96;
    int tileX;
    int tileY;

    float pixelX;
    float pixelY;


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

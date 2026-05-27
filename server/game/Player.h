#pragma once

#include <cstdint>
#include <string>

#include "../../common/dtos/gameTypes.h" 
#include "inventory.h"
#include "raceRepository.h"
#include "classRepository.h"
#include "playerState.h"

class Player {
public:
    Player(uint32_t clientId,
       std::string name,
       const RaceStats& race,
       const ClassStats& cls,
       int16_t maxHp,
       int16_t maxMana);

    bool isAlive() const;
    bool isGhost() const;
    bool isMeditating() const;

    void takeDamage(int16_t dmg);
    void heal(int16_t amount);
    void restoreMana(int16_t amount);

    void addExperience(uint32_t exp);
    void levelUp(int16_t newMaxHp, int16_t newMaxMana);

    void startMeditating();
    void stopMeditating();

    void die();
    void resurrect(int spawnX, int spawnY);

    void tick(float hpGained, float manaGained);

    bool spendMana(int16_t cost);

    int getX() const { 
        return x; 
    }
    
    int getY() const { 
        return y; 
    }
    
    void setPos(int nx, int ny) { 
        x = nx; y = ny; 
    }

    
    uint32_t getClientId() const { return clientId; } // logica en cpp, hay que pasarlo
    const RaceStats&  getRace() const { return race; }
    const ClassStats& getCls()  const { return cls;  }
    uint8_t  getLevel()   const { return level;      }
    int16_t  getHp()      const { return hp;         }
    int16_t  getMaxHp()   const { return maxHp;      }
    int16_t  getMana()    const { return mana;        }
    int16_t  getMaxMana() const { return maxMana;     }
    uint32_t getExp()     const { return experience;  }
    uint32_t getGold()    const { return gold;        }
    uint32_t getId()      const { return clientId;    }

    Player(const Player&)            = delete;
    Player& operator=(const Player&) = delete;

    Player(Player&&)            = default;
    Player& operator=(Player&&) = default;



private:
    int x = 0;
    int y = 0;
    
    uint32_t clientId;
    std::string name;
    const RaceStats& race;
    const ClassStats& cls;

    uint8_t level = 1;
    
    int16_t hp = 0;
    int16_t maxHp = 0;
    int16_t mana = 0;
    int16_t maxMana = 0;
    int gold = 0;
    int experience = 0;

    PlayerState state = PlayerState::ALIVE;

    Inventory inventory;
};
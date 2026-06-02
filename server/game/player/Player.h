#pragma once

#include "../items/EquipSlot.h"
#include "../items/item.h"
#include "../stats/classStats.h"
#include "../stats/raceStats.h"
#include "combatant.h"
#include "inventory.h"
#include "playerState.h"
#include <cstdint>
#include <string>
#include <vector>
#include <toml++/toml.hpp>

class Player : public Combatant {
public:
    Player(uint32_t clientId, std::string name,
           const RaceStats& race, const ClassStats& cls,
           int16_t maxHp, int16_t maxMana,
           const toml::table& config);

    // --- Combatant ---
    uint32_t getId()      const override { return clientId; }
    float    getPixelX()  const override { return pixelX; }
    float    getPixelY()  const override { return pixelY; }
    int      getTileX()   const override;
    int      getTileY()   const override;

    int16_t  getHp()      const override { return hp; }
    int16_t  getMaxHp()   const override { return maxHp; }
    uint8_t  getLevel()   const override { return level; }
    uint8_t  getAgility() const override { return race.agility; }
    uint8_t  getStrength()const override { return race.strength; }
    bool     isAlive()    const override;

    // Rango de ataque en pixeles:
    //   arma ranged => rangedAttackRangePx
    //   melee       => meleeRangePx (aprox 1.5 * tileSize)
    float    getAttackRangePx() const override;

    uint16_t getWeaponDamageMin()  const override;
    uint16_t getWeaponDamageMax()  const override;
    uint16_t getArmorDefenseMin()  const override;
    uint16_t getArmorDefenseMax()  const override;
    uint16_t getHelmetDefenseMin() const override;
    uint16_t getHelmetDefenseMax() const override;
    uint16_t getShieldDefenseMin() const override;
    uint16_t getShieldDefenseMax() const override;

    void takeDamage(int16_t dmg) override;

    // --- Hitbox (AABB centrado en pies) ---
    // left   = pixelX - hitboxW/2
    // right  = pixelX + hitboxW/2
    // top    = pixelY - hitboxH
    // bottom = pixelY
    float getHitboxW() const { return hitboxW; }
    float getHitboxH() const { return hitboxH; }

    // --- Estado ---
    bool isGhost()       const;
    bool isMeditating()  const;
    bool isResurrecting()const;

    // --- Movimiento ---
    // Mueve los pies en pixeles. GameWorld valida colision antes de llamar esto.
    void setPixelPos(float px, float py);

    // --- Stats ---
    void heal(int16_t amount);
    void restoreMana(int16_t amount);
    bool spendMana(int16_t cost);
    void addGold(uint32_t amount);
    void spendGold(uint32_t amount);
    void addExperience(uint32_t exp, uint32_t expLimit,
                       int16_t newMaxHp, int16_t newMaxMana);
    bool checkAndClearLevelUp();

    void startMeditating();
    void stopMeditating();
    void startResurrection();
    void stopResurrection();

    uint32_t die(uint32_t safeGold);
    void resurrect(float px, float py);
    void tick(float hpGained, float manaGained);

    std::vector<Item> purgeInventoryOnDeath();

    // --- Getters ---
    uint32_t getClientId()  const { return clientId; }
    int16_t  getMana()      const { return mana; }
    int16_t  getMaxMana()   const { return maxMana; }
    uint32_t getExp()       const { return experience; }
    uint32_t getGold()      const { return gold; }
    float    getStepPx()    const { return stepPx; }

    const RaceStats&   getRace() const { return race; }
    const ClassStats&  getCls()  const { return cls; }

    Inventory&       getInventory()       { return inventory; }
    const Inventory& getInventory() const { return inventory; }

    void toggleInfiniteHp();
    void toggleInfiniteMana();
    void restoreFullHpAndMana();

private:
    void levelUp(int16_t newMaxHp, int16_t newMaxMana);

    uint32_t clientId;
    std::string name;
    const RaceStats&  race;
    const ClassStats& cls;

    // Posicion — pies del sprite en pixeles
    float pixelX = 0.0f;
    float pixelY = 0.0f;

    // Tamano del tile (para getTileX/Y y rangos)
    int tileSize;

    // Hitbox en pixeles
    float hitboxW;
    float hitboxH;

    // Rango de ataque en pixeles
    float meleeRangePx;
    float rangedAttackRangePx;

    // Cuantos pixeles se mueve por paso (= tileSize normalmente)
    float stepPx;

    int16_t maxHp;
    int16_t maxMana;
    int16_t hp;
    int16_t mana;
    uint32_t experience = 0;
    uint32_t gold       = 0;
    uint8_t  level      = 1;

    bool didLevelUp  = false;
    bool infiniteHp   = false;
    bool infiniteMana = false;

    PlayerState state = PlayerState::ALIVE;
    Inventory   inventory;
};
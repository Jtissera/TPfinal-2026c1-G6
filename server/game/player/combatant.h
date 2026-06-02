#pragma once
#include <cstdint>

class Combatant {
public:
    virtual ~Combatant() = default;

    virtual uint32_t getId()       const = 0;

    // Posicion en pixeles (pies del sprite) — fuente de verdad
    virtual float getPixelX() const = 0;
    virtual float getPixelY() const = 0;

    // Helpers derivados para sistemas que todavia usan tiles
    // (spawn checks, NPC home_range, zone lookup)
    virtual int getTileX() const = 0;
    virtual int getTileY() const = 0;

    virtual int16_t  getHp()     const = 0;
    virtual int16_t  getMaxHp()  const = 0;
    virtual uint8_t  getLevel()  const = 0;
    virtual uint8_t  getAgility()   const = 0;
    virtual uint8_t  getStrength()  const = 0;
    virtual bool     isAlive()      const = 0;

    // Rango de ataque en pixeles
    virtual float getAttackRangePx() const = 0;

    virtual uint16_t getWeaponDamageMin()   const = 0;
    virtual uint16_t getWeaponDamageMax()   const = 0;
    virtual uint16_t getArmorDefenseMin()   const = 0;
    virtual uint16_t getArmorDefenseMax()   const = 0;
    virtual uint16_t getHelmetDefenseMin()  const = 0;
    virtual uint16_t getHelmetDefenseMax()  const = 0;
    virtual uint16_t getShieldDefenseMin()  const = 0;
    virtual uint16_t getShieldDefenseMax()  const = 0;

    virtual void takeDamage(int16_t dmg) = 0;
};
#pragma once
#include <string>
#include <cstdint>

#include "../npcType.h"


enum class Raza : uint8_t {
    HUMAN = 0,
    ELF = 1,
    DWARF = 2,
    GNOME = 3
};

enum class Clase : uint8_t {
    MAGE = 0,
    PALADIN = 1,
    CLERIC = 2,
    WARRIOR = 3
};


struct PlayerDto {
    std::string nombre;
    uint32_t playerID;
    std::string  raza;
    std::string  clase;
    std::string clanName;
    int headId;
    uint8_t level;
    int hp;
    int mana;
    int hpMax;
    int manaMax;
    int oro;
    int oroMax;
    uint16_t xpos;
    uint16_t ypos;
    int exp;
    int expMax;
    bool esFantasma;
    int fuerza;
    int agilidad;
    int inteligencia;
    int constitucion;
};

struct NPCData {
    std::string nombre;
    uint32_t npcID;
    NpcType type;
    uint16_t x;
    uint16_t y;
    int      hp;
    int      hpMax;
    uint16_t level = 0;
    bool estaVivo;
    bool estaMoviendo;
    bool hostile;
};

enum class Direction : uint8_t {
    UP    = 0,
    DOWN  = 1,
    LEFT  = 2,
    RIGHT = 3,
    NONE  = 4
};
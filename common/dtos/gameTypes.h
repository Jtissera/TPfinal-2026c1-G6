
#include <string>
#include <cstdint>

#include "../npcType.h"


enum class Raza : uint8_t {
    HUMANO = 0,
    ELFO = 1,
    ENANO = 2,
    GNOMO = 3
};

enum class Clase : uint8_t {
    MAGO = 0,
    PALADIN = 1,
    CLERIGO = 2,
    GUERRERO = 3
};


struct PlayerDto {
    std::string nombre;
    uint8_t playerID;
    Raza raza;
    Clase clase;
    uint8_t level;
    int hp;
    int mana;
    int hpMax;
    int manaMax;
    int oro;
    int oroMax;
    float xpos;
    float ypos;
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
    uint8_t npcID;
    NpcType type;
    float x;
    float y;
    int      hp;
    int      hpMax;
    bool estaVivo;
    bool estaMoviendo;
};
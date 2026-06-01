
#ifndef TALLER_TP_PLAYERVIEWSTATE_H
#define TALLER_TP_PLAYERVIEWSTATE_H
#include <string>


// PlayerViewState representa el estado visible del jugador en el cliente.
// No reemplaza al modelo del servidor.
// Sirve para que el HUD y otros sistemas SDL lean siempre de una fuente común.
// Cuando el servidor mande actualizaciones de vida, maná, oro o experiencia,
// esas actualizaciones deberían modificar este estado.
enum class PlayerClass {
     Warrior,
     Mage,
     Paladin,
     Cleric,
     Unknown
 };

inline std::string playerClassToString(PlayerClass playerClass) {
    switch (playerClass) {
        case PlayerClass::Warrior:
            return "warrior";
        case PlayerClass::Mage:
            return "mage";
        case PlayerClass::Paladin:
            return "paladin";
        case PlayerClass::Cleric:
            return "cleric";
        case PlayerClass::Unknown:
        default:
            return "unknown";
    }
}

struct PlayerViewState {
     std::string name;
     std::string race;
     PlayerClass playerClass;
     //level
     int level = 1;
     int exp = 0;
     int expToNextLevel = 1000;

     //vida
     int hp = 100;
     int maxHp = 100;

     // mana
     int mana = 100;
     int maxMana = 100;

     // gold
     int gold  = 0;

     bool isDead = false;



};

#endif //TALLER_TP_PLAYERVIEWSTATE_H

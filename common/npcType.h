#pragma once

#include <cstdint>
#include <string>

// Tipos de NPC del juego.
// Usado tanto por el editor (para colocar NPCs en el mapa)
// como por el servidor (para instanciarlos al cargar el mundo).
// NO tocar los valores sin coordinar ambos lados.
enum class NpcType : uint8_t {
    NONE        = 0,   // Sin NPC

    // NPCs de ciudad (zona segura)
    PRIEST      = 1,   // Sacerdote: resucita jugadores
    MERCHANT    = 2,   // Comerciante: compra y vende items
    BANKER      = 3,   // Banquero: deposita y retira oro/items

    // Criaturas (zona de combate)
    GOBLIN      = 10,
    SKELETON    = 11,
    ZOMBIE      = 12,
    GUARD       = 13,
};

// Zona de spawn de npcs (area rectangular donde spawnean criaturas)
struct NpcSpawnZone {
    uint16_t x      = 0;
    uint16_t y      = 0;
    uint16_t width  = 1;
    uint16_t height = 1;
    NpcType  type   = NpcType::GOBLIN;
    uint8_t  maxCount = 3;  // max de NPCs vivos en la zona
};

// Retorna el nombre legible de un NpcType
inline std::string npcTypeKey(NpcType type) {
    switch (type) {
        case NpcType::PRIEST:   return "priest";
        case NpcType::MERCHANT: return "merchant";
        case NpcType::BANKER:   return "banker";
        case NpcType::GOBLIN:   return "goblin";
        case NpcType::SKELETON: return "skeleton";
        case NpcType::ZOMBIE:   return "zombie";
        case NpcType::GUARD:    return "guard";
        default:                return "";
    }
}

inline std::string npcTypeName(NpcType type) {
    switch (type) {
        case NpcType::PRIEST:   return "Sacerdote";
        case NpcType::MERCHANT: return "Mercader";
        case NpcType::BANKER:   return "Banquero";
        case NpcType::GOBLIN:   return "Goblin";
        case NpcType::SKELETON: return "Esqueleto";
        case NpcType::ZOMBIE:   return "Zombi";
        case NpcType::GUARD:    return "Guardia";
        default:                return "NPC";
    }
}
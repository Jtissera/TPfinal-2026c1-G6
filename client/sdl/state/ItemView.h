
#ifndef TALLER_TP_ITEMVIEW_H
#define TALLER_TP_ITEMVIEW_H
#include <cstdint>
#include <string>

// ItemView es la representación del ítem del lado cliente.
// No contiene lógica de negocio pesada.
// Sirve para que SDL sepa cómo mostrar el ítem y qué metadata necesita para equipamiento, consumo y ataques.
// El servidor sigue siendo la autoridad; esta estructura es una vista local para render e interacción.



enum class ClientItemType {
    MeleeWeapon,
    RangedWeapon,
    MagicWeapon,
    Armor,
    Helmet,
    Shield,
    HealthPotion,
    ManaPotion,
    Other       //item no clasificado o decorativo.
};

struct ItemView {
    // ID del catálogo visual.
    // Debe coincidir con assets/items/items.json.
    int itemId = 0;

    // ID único de esta instancia concreta en el servidor.
    // Se usa para pedir equipar, dropear, vender o consumir ESTE item.
    uint32_t instanceId = 0;
    
    // Nombre visible para mostrar en tooltip, debug o UI.
    std::string itemName;

    // Textura usada dentro del inventario.
    std::string textureId;

    // Textura usada cuando el ítem está equipado sobre el personaje.
    std::string visualTextureId;

    // Tipo del ítem para decidir comportamiento.
    ClientItemType type = ClientItemType::Other;


    // Cantidad en inventario.
    // Para equipables normalmente vale 1.
    // Para pociones puede ser mayor.
    int quantity = 1;


    // Daño mínimo y máximo para armas físicas o mágicas.
    int damageMin = 0;
    int damageMax = 0;

    // Defensa mínima y máxima para armaduras, cascos y escudos.
    int defenseMin = 0;
    int defenseMax = 0;

    // Costo de maná para armas mágicas.
    int manaCost = 0;

    // Curación de vida para pociones de vida.
    int healAmount = 0;

    // Recuperación de maná para pociones de maná.
    int manaAmount = 0;
    // Indica si el ataque puede hacerse a distancia.

    bool ranged = false;

    // Sonido asociado al uso del ítem.
    // Ejemplo: sword_hit, magic_cast, drink_potion.
    std::string soundId;
    // Recorte dentro de la spritesheet para dibujar el ícono del inventario.
    int iconSrcX = 0;
    int iconSrcY = 0;
    int iconSrcW = 32;
    int iconSrcH = 32;
    std::string visualTextureIdTall;
    std::string visualTextureIdShort;
    int visualTallOffsetX = 0;
    int visualTallOffsetY = 0;

    int visualShortOffsetX = 0;
    int visualShortOffsetY = 0;
    int visualOffsetX = 0;
    int visualOffsetY = 0;

    // Recortes del sprite equipado según dirección.
    // No son los mismos que el ícono del inventario.
    int visualDownSrcX = 0;
    int visualDownSrcY = 0;
    int visualLeftSrcX = 0;
    int visualLeftSrcY = 0;
    int visualRightSrcX = 0;
    int visualRightSrcY = 0;
    int visualUpSrcX = 0;
    int visualUpSrcY = 0;

    int visualDownOffsetX = 0;
    int visualDownOffsetY = 0;

    int visualLeftOffsetX = 0;
    int visualLeftOffsetY = 0;

    int visualRightOffsetX = 0;
    int visualRightOffsetY = 0;

    int visualUpOffsetX = 0;
    int visualUpOffsetY = 0;
};
#endif //TALLER_TP_ITEMVIEW_H

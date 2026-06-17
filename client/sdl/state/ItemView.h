

// ItemView es la representación del ítem del lado cliente.
// No contiene lógica de negocio pesada.
// Sirve para que SDL sepa cómo mostrar el ítem y qué metadata necesita para equipamiento, consumo y ataques.
// El servidor sigue siendo la autoridad; esta estructura es una vista local para render e interacción.

#ifndef TALLER_TP_ITEMVIEW_H
#define TALLER_TP_ITEMVIEW_H
#include <cstdint>
#include <string>

enum class ClientItemType {
    MeleeWeapon,
    RangedWeapon,
    MagicWeapon,
    Armor,
    Helmet,
    Shield,
    HealthPotion,
    ManaPotion,
    Other
};

struct ItemView {

    // --- Identidad ---
    int itemId = 0;
    uint32_t instanceId = 0;
    std::string itemName;
    ClientItemType type = ClientItemType::Other;

    // --- Ícono en inventario / piso ---
    std::string textureId;
    int iconSrcX = 0;
    int iconSrcY = 0;
    int iconSrcW = 32;
    int iconSrcH = 32;

    // --- Consumibles ---
    int quantity = 1;
    int healAmount = 0;
    int manaAmount = 0;

    // --- Visual equipado (armas, escudos) ---
    std::string visualTextureId;
    int visualOffsetX = 0;
    int visualOffsetY = 0;

    // --- Visual armadura (tall/short según raza) ---
    std::string visualTextureIdTall;
    std::string visualTextureIdShort;
    int visualTallOffsetX = 0;
    int visualTallOffsetY = 0;
    int visualShortOffsetX = 0;
    int visualShortOffsetY = 0;

    // --- Visual casco (recorte por dirección) ---
    int visualSrcW = 32;
    int visualSrcH = 32;
    int visualDownSrcX = 0;
    int visualDownSrcY = 0;
    int visualUpSrcX = 0;
    int visualUpSrcY = 0;
    int visualLeftSrcX = 0;
    int visualLeftSrcY = 0;
    int visualRightSrcX = 0;
    int visualRightSrcY = 0;
};

#endif //TALLER_TP_ITEMVIEW_H

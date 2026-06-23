#include "equipmentDtoFactory.h"

EquipmentDto buildEquipmentDtoFromPlayer(const Player &player)
{
    EquipmentDto dto{};

    const Inventory &inventory = player.getInventory();

    if (const Item *weapon = inventory.getEquipped(EquipSlot::HAND))
    {
        dto.weaponCatalogId = weapon->catalogId;
    }

    if (const Item *armor = inventory.getEquipped(EquipSlot::ARMOR))
    {
        dto.armorCatalogId = armor->catalogId;
    }

    if (const Item *helmet = inventory.getEquipped(EquipSlot::HELMET))
    {
        dto.helmetCatalogId = helmet->catalogId;
    }

    if (const Item *shield = inventory.getEquipped(EquipSlot::SHIELD))
    {
        dto.shieldCatalogId = shield->catalogId;
    }

    return dto;
}
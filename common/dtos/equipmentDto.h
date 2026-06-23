
#ifndef TALLER_TP_EQUIPMENTDTO_H
#define TALLER_TP_EQUIPMENTDTO_H
#include <cstdint>

struct EquipmentDto
{
    uint32_t weaponCatalogId = 0;
    uint32_t armorCatalogId = 0;
    uint32_t helmetCatalogId = 0;
    uint32_t shieldCatalogId = 0;

    bool weaponCanHeal = false;
};

#endif

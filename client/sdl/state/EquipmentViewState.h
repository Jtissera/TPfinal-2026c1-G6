
#ifndef TALLER_TP_EQUIPMENTVIEWSTATE_H
#define TALLER_TP_EQUIPMENTVIEWSTATE_H
#pragma once

#include <optional>

#include "ItemView.h"

//EquipmentViewState representa los ítems actualmente equipados por el jugador.
//Está separado del inventario porque los ítems equipados no ocupan slots de mochila.
//Uso optional porque cada slot puede estar vacío u ocupado.

struct EquipmentViewState {
    std::optional<ItemView>  weapon;
    std::optional<ItemView>  helmet;
    std::optional<ItemView>  armor;
    std::optional<ItemView>  shield;

};
enum class ClientEquipmentSlot : int {
    Weapon = 0,
    Helmet = 1,
    Armor = 2,
    Shield = 3
};
#endif //TALLER_TP_EQUIPMENTVIEWSTATE_H

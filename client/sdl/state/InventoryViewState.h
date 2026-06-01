
#ifndef TALLER_TP_INVENTORYVIEWSTATE_H
#define TALLER_TP_INVENTORYVIEWSTATE_H


#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "ItemView.h"

// Estado visual del inventario del cliente.
//
// Contiene los ítems que actualmente se muestran en los slots.
// Si un ítem está equipado, no debería estar acá visualmente,
// aunque el servidor internamente pueda seguir considerándolo parte del inventario.
struct InventoryViewState {
    // Ítems visibles en el inventario.
    std::vector<std::optional<ItemView>> slots;

    explicit InventoryViewState(std::size_t maxSlots = 20) :slots(maxSlots,std::nullopt){}

    // Indica si todavía hay espacio para agregar un ítem visualmente.
    bool hasFreeSlot() const {
        for (const auto& slot : slots) {
            if (!slot.has_value()) {
                return  true;
            }
        }
        return  false;
    }


    int firstFreeSlotIndex() const {
        for (std::size_t i = 0; i < slots.size(); i++) {
            if (!slots[i].has_value()) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }



};







#endif //TALLER_TP_INVENTORYVIEWSTATE_H

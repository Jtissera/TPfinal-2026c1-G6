
#ifndef TALLER_TP_ITEMCATALOG_H_H
#define TALLER_TP_ITEMCATALOG_H_H

#pragma once

#include <string>
#include <unordered_map>

#include "../state/ItemView.h"

// Catálogo de ítems del cliente.
//
// Su responsabilidad es guardar la metadata necesaria para que SDL pueda:
// - dibujar ítems en inventario;
// - dibujar equipamiento;
// - saber daño, defensa, manaCost y sonidos;
// - construir estados visuales a partir de ids recibidos del servidor.


class ItemCatalog {
public:
    void loadFromJson(const std::string& path);

    void addItem(const ItemView& item);

    bool contains(int itemId) const;

    const ItemView* getById(int itemId) const;

    const ItemView& requireById(int itemId) const;

private:
    ClientItemType parseItemType(const std::string& type) const;

    std::unordered_map<int, ItemView> itemsById;
};
#endif //TALLER_TP_ITEMCATALOG_H_H

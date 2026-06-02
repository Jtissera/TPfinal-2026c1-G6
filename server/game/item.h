#pragma once
#include "itemEffect.h"
#include "itemSlot.h"
#include "itemStats.h"
#include <cstdint>
#include <string>

struct Item {
  // Sirve para equipar, dropear, vender o consumir este item puntual.
  uint32_t instanceId = 0;

  // ID estable del catálogo compartido con el cliente.
  // Debe coincidir con assets/items/items.json.
  uint32_t catalogId = 0;
  std::string typeName;
  ItemSlot slot = ItemSlot::NONE;
  ItemEffect effect = ItemEffect::NONE;
  ItemStats stats;

  bool isValid() const { return slot != ItemSlot::NONE; }
};
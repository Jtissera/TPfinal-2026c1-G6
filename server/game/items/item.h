#pragma once

#include "itemEffect.h"
#include "itemSlot.h"
#include "itemStats.h"
#include <cstdint>
#include <string>

struct Item
{
  uint32_t instanceId = 0;
  uint32_t catalogId = 0;
  std::string typeName;
  ItemSlot slot = ItemSlot::NONE;
  ItemEffect effect = ItemEffect::NONE;
  ItemStats stats;

  bool isValid() const;
};
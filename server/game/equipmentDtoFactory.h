#pragma once

#include "common/dtos/equipmentDto.h"
#include "server/game/player/Player.h"
#include "server/game/items/EquipSlot.h"
#include "server/game/player/inventory.h"
#include "server/game/items/item.h"

EquipmentDto buildEquipmentDtoFromPlayer(const Player &player);
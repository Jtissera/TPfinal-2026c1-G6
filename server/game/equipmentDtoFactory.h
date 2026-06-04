
#ifndef TALLER_TP_EQUIPMENTDTOFACTORY_H
#define TALLER_TP_EQUIPMENTDTOFACTORY_H


#pragma once

#include "common/dtos/equipmentDto.h"
#include "server/game/Player.h"

EquipmentDto buildEquipmentDtoFromPlayer(const Player& player);

#endif //TALLER_TP_EQUIPMENTDTOFACTORY_H

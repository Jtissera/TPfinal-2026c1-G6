//
// Created by mauro on 2/6/26.
//

#ifndef TALLER_TP_REMOTEPLAYER_H
#define TALLER_TP_REMOTEPLAYER_H

#include <cstdint>

#include "../ECS/ECS.h"
#include "../ECS/Components.h"
#include "common/dtos/equipmentDto.h"

class RemotePlayer {
private:
    // Id del jugador remoto según el servidor.
    uint32_t id;

    // Puntero NO dueño a la entidad ECS.
    // La entidad la administra Manager, no RemotePlayer.
    Entity* entity;

public:
    // Constructor: asocia un id remoto con una entidad visual.
    RemotePlayer(uint32_t id, Entity* entity);


    uint32_t getId() const;

    // Versión mutable: permite modificar la Entity.
    Entity* getEntity();

    // Versión const: permite leer desde un RemotePlayer const.
    const Entity* getEntity() const;

    // Actualiza la posición visual del jugador remoto.
    // Actualiza posición y animación visual del jugador remoto.
    void setPositionAndAnimation(float x, float y, Direction direction, bool moving);

    void setEquipment(const EquipmentDto& equipment, const ItemCatalog& itemCatalog);
};


#endif //TALLER_TP_REMOTEPLAYER_H

//
// Created by mauro on 2/6/26.
//

#ifndef TALLER_TP_REMOTEPLAYER_H
#define TALLER_TP_REMOTEPLAYER_H

#include <cstdint>

#include "../ECS/ECS.h"
#include "../ECS/Components.h"

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

    // Devuelve el id de red del jugador remoto.
    uint32_t getId() const;

    // Devuelve la entidad visual asociada.
    Entity* getEntity() const;

    // Actualiza la posición visual del jugador remoto.
    void setPosition(float x, float y);
};


#endif //TALLER_TP_REMOTEPLAYER_H

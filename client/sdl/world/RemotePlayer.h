//
// Created by mauro on 2/6/26.
//

#ifndef TALLER_TP_REMOTEPLAYER_H
#define TALLER_TP_REMOTEPLAYER_H

#include <cstdint>

#include "../ECS/ECS.h"
#include "../ECS/Components.h"
#include "common/dtos/equipmentDto.h"
#include "common/dtos/gameTypes.h"

class RemotePlayer {
private:
    // Id del jugador remoto según el servidor.
    uint32_t id;

    // Puntero NO dueño a la entidad ECS.
    // La entidad la administra Manager, no RemotePlayer.
    Entity* entity;

    bool ghost = false;

    // Convierte una dirección en el sufijo usado por las animaciones.
    // Ejemplo: Direction::LEFT -> "Left".
    const char* directionSuffix(Direction direction) const;

    // Arma el nombre completo de la animación.
    std::string animationNameFor(Direction direction, bool moving) const;

    PlayerDto dto;

    void setLevel(uint32_t newLevel);
    void setClan(const std::string& newClan);


public:
    // Constructor: asocia un id remoto con una entidad visual.
    RemotePlayer(uint32_t id, Entity* entity, const PlayerDto& dto);


    uint32_t getId() const;

    // Versión mutable: permite modificar la Entity.
    Entity* getEntity();

    // Versión const: permite leer desde un RemotePlayer const.
    const Entity* getEntity() const;

    // Actualiza la posición visual del jugador remoto.
    // Actualiza posición y animación visual del jugador remoto.
    void setPositionAndAnimation(float x, float y, Direction direction, bool moving);

    void setEquipment(const EquipmentDto& equipment, const ItemCatalog& itemCatalog);

    bool isGhost() const;
    void setGhost(bool value);

    const PlayerDto& getDto() const;

    void updateDto(const PlayerDto& newDto);
    uint8_t getLevel() const;
    void setLevel(uint8_t newLevel);
    void setHealth(int hp, int hpMax);
};


#endif //TALLER_TP_REMOTEPLAYER_H

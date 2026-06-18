//
// Created by mauro on 18/6/26.
//
#ifndef TALLER_TP_PICKUPSYSTEM_H
#define TALLER_TP_PICKUPSYSTEM_H

#include <SDL2/SDL_rect.h>
#include <map>
#include <cstdint>
#include "ECS/ECS.h"
#include "common/queue.h"
#include "common/network/messages/message.h"

struct GroundPickupTarget {
    uint32_t instanceId;
    bool isGold;
    Entity* entity;
};

class PickupSystem {
public:
    PickupSystem() = default;

    // Detecta click sobre un item/oro del piso y manda PickItemMessage.
    // Devuelve true si efectivamente clickeo algo pickeable

    bool handleMouseClick(int screenX,int screenY,const std::vector<GroundPickupTarget>& targets,
        Queue<std::shared_ptr<const Message>>* sendQueue );

private:
    void sendPickMessage(uint32_t instanceId, bool isGold, Queue<std::shared_ptr<const Message>>* sendQueue);
};

#endif
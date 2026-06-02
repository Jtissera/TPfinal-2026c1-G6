#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include "../common/queue.h"
#include "../common/network/messages/message.h"
#include "../game/player/Player.h"

struct InstanceTransitionEvent
{
    uint32_t clientId;
    uint32_t fromRoomId;
    Player player;
    Queue<std::shared_ptr<const Message>> *clientQueue;
    std::string targetMap; // vacío si es salida (EXIT tile)
    int spawnTileX = 6;
    int spawnTileY = 7;
};
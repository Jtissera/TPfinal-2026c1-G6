#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../common/queue.h"
#include "../common/network/messages/message.h"
#include "../game/player/Player.h"

struct InstanceTransitionEvent
{
    uint32_t clientId;
    uint32_t fromRoomId;
    Player player;
    Queue<std::shared_ptr<const Message>> *clientQueue;
    std::string targetMap;
    int spawnTileX = 6;
    int spawnTileY = 7;
};
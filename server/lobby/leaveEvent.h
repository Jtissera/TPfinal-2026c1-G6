#pragma once
#include "../../common/network/messages/message.h"
#include "../../common/queue.h"
#include "../game/player/Player.h"
#include <cstdint>
#include <memory>

struct LeaveEvent {
  uint32_t clientId;
  uint32_t gameId;
  Player player;
  Queue<std::shared_ptr<const Message>> *clientQueue;
};
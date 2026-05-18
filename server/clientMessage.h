#pragma once

#include <cstdint>
#include <memory>

#include "../common/network/messages/message.h"

struct ClientMessage
{
    uint32_t clientId;
    std::shared_ptr<Message> message;
};

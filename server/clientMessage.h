#pragma once

#include <cstdint>
#include <memory>

#include "../common/network/messages/message.h"

struct ClientMessage
{
    uint32_t clientId;
    std::unique_ptr<Message> message;
};
